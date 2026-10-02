"""Map side-space coordinates to gantry coordinates for each arm direction."""
from . import config

class OutOfRangeError(ValueError):
    """Raised when a mapped machine coordinate falls outside machine travel."""


def _check_range(value, axis_name, bounds):
    lower, upper = bounds[axis_name.lower()]
    if value < lower or value > upper:
        raise OutOfRangeError(
            f"machine {axis_name}={value}mm is outside travel "
            f"[{lower}, {upper}]"
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
    if orientation not in config.ORIENTATION_ANGLES:
        raise ValueError(
            f"Unknown orientation {orientation!r}, expected one of {sorted(config.ORIENTATION_ANGLES)}"
        )

    if machine_travel is None:
        machine_travel = {
            "x": axis_bounds["x"][1] if axis_bounds else config.TABLE_SIDE_MM,
            "y": axis_bounds["y"][1] if axis_bounds else config.TABLE_SIDE_MM,
        }
    arm_reach = config.ARM_REACH_MM if arm_reach is None else arm_reach
    result = {}
    if orientation == 1:  # Facing +Y; local Y is measured from the far edge.
        if x is not None:
            result["x"] = x
        if y is not None:
            result["y"] = y - arm_reach + machine_travel["y"]
    elif orientation == 2:  # Facing +X; swap axes, retaining the side offset.
        if x is not None:
            result["y"] = x - arm_reach + machine_travel["y"]
        if y is not None:
            result["x"] = y
    elif orientation == 3:  # Facing -Y.
        if x is not None:
            result["x"] = x
        if y is not None:
            result["y"] = y + arm_reach
    else:  # orientation == 4, facing -X.
        if x is not None:
            result["y"] = x
        if y is not None:
            result["x"] = y + arm_reach

    if axis_bounds is not None:
        for axis, value in result.items():
            _check_range(value, axis.upper(), axis_bounds)
    return result


def orientation_angle(orientation):
    try:
        return config.ORIENTATION_ANGLES[orientation] + config.ROTATION_OFFSET_DEG
    except KeyError:
        raise ValueError(
            f"Unknown orientation {orientation!r}, expected one of {sorted(config.ORIENTATION_ANGLES)}"
        ) from None
