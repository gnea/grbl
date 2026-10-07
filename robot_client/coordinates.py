"""Map side-space coordinates to gantry coordinates for each arm direction."""
from typing import Callable, Dict, NamedTuple, Tuple

from . import config


class OutOfRangeError(ValueError):
    """Raised when a mapped machine coordinate falls outside machine travel."""


class Side(NamedTuple):
    angle: float  # A-axis angle from the home direction
    matrix: Tuple[int, int, int, int]  # (a, b, c, d): X = a*x' + b*y' + ox, Y = c*x' + d*y' + oy
    offset: Callable[[float, float, float], Tuple[float, float]]  # (max_x, max_y, arm) -> (ox, oy)
    arm: Tuple[int, int]  # unit vector from carriage to gripper tip


#   1: X = Y' + (max_x - L)   Y = max_y - X'
#   2: X = X'                 Y = Y' + (max_y - L)
#   3: X = -Y' + L            Y = X'
#   4: X = -X' + max_x        Y = -Y' + L
SIDES: Dict[int, Side] = {
    1: Side(0.0, (0, 1, -1, 0), lambda mx, my, L: (mx - L, my), (1, 0)),
    2: Side(90.0, (1, 0, 0, 1), lambda mx, my, L: (0.0, my - L), (0, 1)),
    3: Side(180.0, (0, -1, 1, 0), lambda mx, my, L: (L, 0.0), (-1, 0)),
    4: Side(270.0, (-1, 0, 0, -1), lambda mx, my, L: (mx, L), (0, -1)),
}


def _side(orientation) -> Side:
    try:
        return SIDES[orientation]
    except KeyError:
        raise ValueError(
            f"Unknown orientation {orientation!r}, expected one of {sorted(SIDES)}"
        ) from None


def _check_range(value, axis, bounds):
    lower, upper = bounds[axis]
    if value < lower or value > upper:
        raise OutOfRangeError(
            f"machine {axis.upper()}={value}mm is outside travel [{lower}, {upper}]"
        )


def local_to_machine_axes(
    orientation,
    x=None,
    y=None,
    axis_bounds=None,
    machine_travel=None,
    arm_reach=None,
):
    """Map side-space coordinates to machine axes, applying arm reach offset.

    Only axes actually passed in (not None) are mapped and returned, so
    callers can move X, Y, or both independently.

    Returns a dict such as {"x": mm} or {"x": mm, "y": mm}.
    """
    side = _side(orientation)
    if machine_travel is None:
        machine_travel = {
            "x": axis_bounds["x"][1] if axis_bounds else config.TABLE_SIDE_MM,
            "y": axis_bounds["y"][1] if axis_bounds else config.TABLE_SIDE_MM,
        }
    arm_reach = config.ARM_REACH_MM if arm_reach is None else arm_reach
    a, b, c, d = side.matrix
    x_offset, y_offset = side.offset(machine_travel["x"], machine_travel["y"], arm_reach)

    result = {}
    if x is not None:
        if a:
            result["x"] = a * x + x_offset
        if c:
            result["y"] = c * x + y_offset
    if y is not None:
        if b:
            result["x"] = b * y + x_offset
        if d:
            result["y"] = d * y + y_offset

    if axis_bounds is not None:
        for axis, value in result.items():
            _check_range(value, axis, axis_bounds)
    return result


def side_to_gripper_xy(orientation, x, y, machine_travel, arm_reach=None):
    """Return the table-plane point reached by a side-space XY target."""
    reach = config.ARM_REACH_MM if arm_reach is None else arm_reach
    base = local_to_machine_axes(
        orientation, x=x, y=y, machine_travel=machine_travel, arm_reach=reach
    )
    dx, dy = _side(orientation).arm
    return base["x"] + dx * reach, base["y"] + dy * reach


def orientation_angle(orientation):
    return _side(orientation).angle
