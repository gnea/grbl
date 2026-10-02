"""High-level robot API: connect, home, change orientation, move, transport."""
from typing import Optional, Tuple

from . import config
from .coordinates import local_to_machine_axes, orientation_angle
from .grbl_link import GrblLink


class RobotController:
    def __init__(self, port=None, baud=None):
        self.link = GrblLink(port or config.DEFAULT_PORT, baud or config.SERIAL_BAUD)
        self.orientation: Optional[int] = None

    def connect(self):
        self.link.connect()
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
        self.link.wait_idle()
        # Homing does not change the gripper's current orientation.

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
        self.link.send_line("G90")
        self.link.send_line(" ".join(parts))
        self.link.wait_idle()

    def set_orientation(self, orientation: int):
        """Park the arm at mid-reach on X/Y, then rotate the gripper (A axis) to face a side."""
        angle = orientation_angle(orientation)
        self._move(
            x=config.ORIENTATION_CHANGE_X,
            y=config.ORIENTATION_CHANGE_Y,
            feed=config.DEFAULT_FEED_RATE_XY,
        )
        self._move(a=angle, feed=config.DEFAULT_FEED_RATE_A)
        self.orientation = orientation

    def move_to(self, x=None, y=None, z=None):
        """Move to a table-local position, mapped for the current orientation.

        Any of x, y, z may be omitted to move only the specified axes.
        """
        if self.orientation is None:
            raise RuntimeError("Call set_orientation() before moving to table coordinates")

        machine_axes = {}
        if x is not None or y is not None:
            machine_axes = local_to_machine_axes(self.orientation, x=x, y=y)

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
        pick_xy: Tuple[float, float],
        place_xy: Tuple[float, float],
        z_travel: float,
        z_pick: float,
        z_place: float,
    ):
        """Move a plate from one table side to another."""
        pick_x, pick_y = pick_xy
        place_x, place_y = place_xy

        self.set_orientation(from_orientation)
        self.move_to(x=pick_x, y=pick_y)
        self._move(z=z_pick, feed=config.DEFAULT_FEED_RATE_Z)
        # gripper.close() goes here once the gripper hardware is implemented.
        self._move(z=z_travel, feed=config.DEFAULT_FEED_RATE_Z)

        self.set_orientation(to_orientation)
        self.move_to(x=place_x, y=place_y)
        self._move(z=z_place, feed=config.DEFAULT_FEED_RATE_Z)
        # gripper.open() goes here once the gripper hardware is implemented.
        self._move(z=z_travel, feed=config.DEFAULT_FEED_RATE_Z)
