"""High-level robot API: connect, home, change orientation, move, transport."""
from typing import Dict, List, Optional, Tuple

from . import config
from .coordinates import (
    OutOfRangeError,
    local_to_machine_axes,
    orientation_angle,
    side_to_gripper_xy,
)
from .crpt_link import CrptError, CrptLink, MachineLink
from .machine import AxisBounds, MachineSettings, compute_axis_bounds


class RobotController:
    def __init__(
        self,
        port=None,
        baud=None,
        sim=False,
        simulator_url: Optional[str] = None,
        open_simulator=True,
        homing_rotation_offset_deg=None,
        link: Optional[MachineLink] = None,
    ):
        """Pass `link` to inject any MachineLink; otherwise `sim` picks simulator or serial."""
        if homing_rotation_offset_deg is None:
            homing_rotation_offset_deg = 0.0 if sim else config.HOMING_ROTATION_OFFSET_DEG
        self.homing_rotation_offset_deg = float(homing_rotation_offset_deg)
        self.a_home_position = 0.0
        self.link: MachineLink = link or self._build_link(
            port, baud, sim, simulator_url, open_simulator
        )
        self.orientation: Optional[int] = None
        self.settings: Optional[MachineSettings] = None
        self.axis_bounds: Optional[AxisBounds] = None
        self.machine_position: Optional[List[float]] = None

    @staticmethod
    def _build_link(port, baud, sim, simulator_url, open_simulator) -> MachineLink:
        if sim:
            from .simulation.simulator import SimulatorLink

            return SimulatorLink(url=simulator_url, open_browser=open_simulator)
        return CrptLink(port or config.DEFAULT_PORT, baud or config.SERIAL_BAUD)

    def connect(self):
        self.link.connect()
        # self.home()
        self._load_machine_configuration()
        self._update_machine_position()
        # self.link.send_line("$X")
        return self

    def close(self):
        self.link.close()

    def __enter__(self):
        return self.connect()

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def home(self):
        self.link.home()
        status = self.link.wait_idle()
        self._load_machine_configuration()
        position = self._update_machine_position(status)

        home_a = -self.homing_rotation_offset_deg
        lower_a, upper_a = self._require_bounds()["a"]
        if not lower_a <= home_a <= upper_a:
            raise OutOfRangeError(
                f"Homing correction A{home_a:g} is outside configured travel "
                f"[{lower_a:g}, {upper_a:g}]"
            )
        if home_a != position[3]:
            self._move(a=home_a, feed=config.DEFAULT_FEED_RATE_A)
        self.a_home_position = home_a
        self.set_orientation(1)  # raises Z to max
        self.move_to(x=0.0, y=0.0)

    def _load_machine_configuration(self):
        settings = self.link.read_settings()
        self.axis_bounds = compute_axis_bounds(settings)
        self.settings = settings

    def _update_machine_position(self, status=None) -> List[float]:
        status = status or self.link.get_status()
        values = status.get("MPos")
        if values is None:
            raise CrptError("GRBL-CRPT status does not include MPos; enable machine-position reporting")
        self.machine_position = [float(value) for value in values.split(",")]
        return self.machine_position

    def unlock(self):
        self.link.unlock()

    def _require_bounds(self) -> AxisBounds:
        if self.axis_bounds is None or self.settings is None:
            raise RuntimeError("Home first so the client can read travel and pull-off settings")
        return self.axis_bounds

    def _machine_travel(self) -> Dict[str, float]:
        bounds = self._require_bounds()
        return {"x": bounds["x"][1], "y": bounds["y"][1]}

    def _move(self, x=None, y=None, z=None, a=None, feed=None):
        targets = {"x": x, "y": y, "z": z, "a": a}
        targets = {axis: value for axis, value in targets.items() if value is not None}
        if not targets:
            return
        bounds = self._require_bounds()
        for axis, value in targets.items():
            lower, upper = bounds[axis]
            if not lower <= value <= upper:
                raise OutOfRangeError(
                    f"{axis.upper()} target {value} is outside allowed range [{lower}, {upper}]"
                )
        parts = ["G1" if feed is not None else "G0"]
        parts += [f"{axis.upper()}{value:.3f}" for axis, value in targets.items()]
        if feed is not None:
            parts.append(f"F{feed:.1f}")
        self.link.send_line("G21 G90")
        self.link.send_line(" ".join(parts))
        self._update_machine_position(self.link.wait_idle())

    def _nearest_a(self, orientation: int) -> float:
        """A-axis target for an orientation, choosing the shortest rotation within travel."""
        lower, upper = self._require_bounds()["a"]
        base = self.a_home_position + orientation_angle(orientation)
        candidates = [
            base + 360.0 * turn
            for turn in range(-1, int(upper // 360.0) + 2)
            if lower <= base + 360.0 * turn <= upper
        ]
        if not candidates:
            raise OutOfRangeError(f"Orientation {orientation} is outside A-axis travel")
        current_a = (self.machine_position or self._update_machine_position())[3]
        return min(candidates, key=lambda candidate: abs(candidate - current_a))

    def set_orientation(self, orientation: int):
        """Raise Z and rotate to the requested side without changing X/Y."""
        self._move(z=self._require_bounds()["z"][1], feed=config.DEFAULT_FEED_RATE_Z)
        target_a = self._nearest_a(orientation)
        assert self.machine_position is not None
        if abs(target_a - self.machine_position[3]) > 1e-9:
            self._move(a=target_a, feed=config.DEFAULT_FEED_RATE_A)
        self.orientation = orientation

    def rotate_a_to(self, angle):
        """Move A to an absolute angle within its full configured travel."""
        self._move(a=angle, feed=config.DEFAULT_FEED_RATE_A)

    def move_to(self, x=None, y=None, z=None):
        """Move to a table-local position, mapped for the current orientation.

        Any of x, y, z may be omitted to move only the specified axes.
        """
        if self.orientation is None:
            raise RuntimeError("Call set_orientation() before moving to table coordinates")
        machine_axes = {}
        if x is not None or y is not None:
            machine_axes = local_to_machine_axes(
                self.orientation,
                x=x,
                y=y,
                axis_bounds=self._require_bounds(),
                machine_travel=self._machine_travel(),
            )
        self._move(
            x=machine_axes.get("x"),
            y=machine_axes.get("y"),
            z=z,
            feed=config.DEFAULT_FEED_RATE_XY,
        )

    def move_machine_to(self, x=None, y=None, z=None):
        """Move directly to absolute machine-axis coordinates within safe bounds."""
        z_only = z is not None and x is None and y is None
        feed = config.DEFAULT_FEED_RATE_Z if z_only else config.DEFAULT_FEED_RATE_XY
        self._move(x=x, y=y, z=z, feed=feed)

    def transport(
        self,
        from_orientation: int,
        to_orientation: int,
        pick_xyz: Tuple[float, float, float],
        place_xyz: Tuple[float, float, float],
    ):
        """Move a plate from one table side to another."""
        z_up = self._require_bounds()["z"][1]
        travel = self._machine_travel()
        pick_x, pick_y, pick_z = pick_xyz
        place_x, place_y, place_z = place_xyz
        self.link.prepare_transport(
            side_to_gripper_xy(from_orientation, pick_x, pick_y, travel),
            side_to_gripper_xy(to_orientation, place_x, place_y, travel),
        )
        self._move(z=z_up, feed=config.DEFAULT_FEED_RATE_Z)
        self.move_machine_to(x=travel['x']/2, y=travel['y']/2)
        self.set_orientation(from_orientation)
        self.move_to(x=pick_x, y=pick_y)
        self._move(z=pick_z, feed=config.DEFAULT_FEED_RATE_Z)
        self.link.gripper_close()
        self._move(z=z_up, feed=config.DEFAULT_FEED_RATE_Z)

        self.move_machine_to(x=travel['x']/2, y=travel['y']/2)
        self.set_orientation(to_orientation)
        self.move_to(x=place_x, y=place_y)
        self._move(z=place_z, feed=config.DEFAULT_FEED_RATE_Z)
        self.link.gripper_open()
        self._move(z=z_up, feed=config.DEFAULT_FEED_RATE_Z)
        self.move_machine_to(x=travel['x']/2, y=travel['y']/2)
