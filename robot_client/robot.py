"""High-level robot API: connect, home, change orientation, move, transport."""
from typing import Optional, Tuple

from . import config
from .coordinates import OutOfRangeError, local_to_machine_axes, orientation_angle
from .grbl_link import GrblError, GrblLink


class RobotController:
    def __init__(self, port=None, baud=None):
        self.link = GrblLink(port or config.DEFAULT_PORT, baud or config.SERIAL_BAUD)
        self.orientation: Optional[int] = None
        self.settings = None
        self.axis_bounds = None
        self.machine_position = None

    def connect(self):
        self.link.connect()
        self.home()
        return self

    def close(self):
        self.link.close()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def home(self):
        self.link.home()
        status = self.link.wait_idle()
        self._load_machine_configuration()
        self._update_machine_position(status)
        self.orientation = None
        self.set_orientation(1)

    def _load_machine_configuration(self):
        settings = self.link.read_settings()
        required = [28, 130, 131, 132, 133, 140, 141, 142, 143]
        missing = [number for number in required if number not in settings]
        if missing:
            raise GrblError(
                "Firmware is missing robot travel/pull-off settings: "
                + ", ".join(f"${number}" for number in missing)
                + ". Flash the updated robot firmware first."
            )

        maximum_end_mask = int(settings[28])
        bounds = {}
        for axis, bit, travel_setting, pull_off_setting in (
            ("x", 1, 130, 140),
            ("y", 2, 131, 141),
            ("z", 4, 132, 142),
        ):
            travel = settings[travel_setting]
            pull_off = settings[pull_off_setting]
            if travel <= 0 or pull_off < 0 or pull_off >= travel:
                raise GrblError(f"Invalid travel/pull-off configuration for {axis.upper()}")
            if maximum_end_mask & bit:
                bounds[axis] = (0.0, travel - pull_off)
            else:
                bounds[axis] = (pull_off, travel)

        a_travel = settings[133]
        if a_travel <= 0:
            raise GrblError("Invalid A-axis travel setting $133")
        bounds["a"] = (0.0, a_travel)
        self.settings = settings
        self.axis_bounds = bounds

    def _update_machine_position(self, status=None):
        status = status or self.link.get_status()
        values = status.get("MPos")
        if values is None:
            raise GrblError("Grbl status does not include MPos; enable machine-position reporting")
        self.machine_position = [float(value) for value in values.split(",")]

    def unlock(self):
        self.link.unlock()

    def _move(self, x=None, y=None, z=None, a=None, feed=None):
        parts = ["G1" if feed is not None else "G0"]
        if x is not None:
            parts.append(f"X{x:.3f}")
        if y is not None:
            parts.append(f"Y{y:.3f}")
        if z is not None:
            parts.append(f"Z{z:.3f}")
        if a is not None:
            parts.append(f"A{a:.3f}")
        if feed is not None:
            parts.append(f"F{feed:.1f}")
        if len(parts) == 1:
            return  # nothing to move
        if self.axis_bounds is None or self.settings is None:
            raise RuntimeError("Home first so the client can read travel and pull-off settings")
        for axis, value in (("x", x), ("y", y), ("z", z), ("a", a)):
            if value is not None:
                lower, upper = self.axis_bounds[axis]
                if value < lower or value > upper:
                    raise OutOfRangeError(
                        f"{axis.upper()} target {value} is outside allowed range [{lower}, {upper}]"
                    )
        self.link.send_line("G21 G90")
        self.link.send_line(" ".join(parts))
        status = self.link.wait_idle()
        self._update_machine_position(status)

    def set_orientation(self, orientation: int):
        """Raise Z, park X/Y at mid-travel, then rotate to the calibrated side."""
        if self.axis_bounds is None or self.settings is None:
            raise RuntimeError("Home first so the client can read travel and pull-off settings")
        angle = orientation_angle(orientation)
        center_x = sum(self.axis_bounds["x"]) / 2.0
        center_y = sum(self.axis_bounds["y"]) / 2.0
        self._move(z=self.axis_bounds["z"][1], feed=config.DEFAULT_FEED_RATE_Z)
        self._move(
            x=center_x,
            y=center_y,
            feed=config.DEFAULT_FEED_RATE_XY,
        )
        if self.machine_position is None:
            self._update_machine_position()
        if self.machine_position is None:
            raise RuntimeError("Could not read current machine position")
        current_a = self.machine_position[3]
        lower_a, upper_a = self.axis_bounds["a"]
        candidates = [
            angle + 360.0 * turn
            for turn in range(-1, int(upper_a // 360.0) + 2)
            if lower_a <= angle + 360.0 * turn <= upper_a
        ]
        if not candidates:
            raise OutOfRangeError(f"Orientation {orientation} is outside A-axis travel")
        target_a = min(candidates, key=lambda candidate: abs(candidate - current_a))
        self._move(a=target_a, feed=config.DEFAULT_FEED_RATE_A)
        self.orientation = orientation

    def rotate_a_to(self, angle):
        """Move A to an absolute angle within its full configured travel."""
        if self.axis_bounds is None:
            raise RuntimeError("Home first so the client can read A-axis travel")
        self._move(a=angle, feed=config.DEFAULT_FEED_RATE_A)

    def move_to(self, x=None, y=None, z=None):
        """Move to a table-local position, mapped for the current orientation.

        Any of x, y, z may be omitted to move only the specified axes.
        """
        if self.orientation is None:
            raise RuntimeError("Call set_orientation() before moving to table coordinates")
        if self.axis_bounds is None or self.settings is None:
            raise RuntimeError("Home first so the client can read travel and pull-off settings")

        machine_axes = {}
        if x is not None or y is not None:
            machine_axes = local_to_machine_axes(
                self.orientation,
                x=x,
                y=y,
                axis_bounds=self.axis_bounds,
                machine_travel={
                    "x": self.settings[130],
                    "y": self.settings[131],
                },
            )

        self._move(
            x=machine_axes.get("x"),
            y=machine_axes.get("y"),
            z=z,
            feed=config.DEFAULT_FEED_RATE_XY,
        )

    def transport(
        self,
        from_orientation: int,
        to_orientation: int,
        pick_xyz: Tuple[float, float, float],
        place_xyz: Tuple[float, float, float],
    ):
        """Move a plate from one table side to another."""
        if self.axis_bounds is None:
            raise RuntimeError("Home first so the client can read travel and pull-off settings")

        pick_x, pick_y, pick_z = pick_xyz
        place_x, place_y, place_z = place_xyz
        z_up = self.axis_bounds["z"][1]

        self.set_orientation(from_orientation)
        self.move_to(x=pick_x, y=pick_y)
        self._move(z=pick_z, feed=config.DEFAULT_FEED_RATE_Z)
        # gripper.close() goes here once the gripper hardware is implemented.
        self._move(z=z_up, feed=config.DEFAULT_FEED_RATE_Z)

        self.set_orientation(to_orientation)
        self.move_to(x=place_x, y=place_y)
        self._move(z=place_z, feed=config.DEFAULT_FEED_RATE_Z)
        # gripper.open() goes here once the gripper hardware is implemented.
        self._move(z=z_up, feed=config.DEFAULT_FEED_RATE_Z)
