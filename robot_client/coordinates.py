"""Table-local <-> machine coordinate mapping.

Orientation 1 is the gantry's native frame (local X drives machine X, local
Y drives machine Y). Orientation 2 is rotated 90 degrees: local X drives
machine Y, and local Y drives machine X. Orientation 3 is rotated 180
degrees, which swaps back to the unswapped mapping of orientation 1.
"""
from . import config

# True for orientations where local X/Y must be swapped to get machine X/Y.
_AXIS_SWAP = {
    1: False,
    2: True,
    3: False,
}


class OutOfRangeError(ValueError):
    """Raised when a mapped machine coordinate falls outside machine travel."""


def _check_range(value, axis_name):
    if value < config.MACHINE_MIN_MM or value > config.MACHINE_MAX_MM:
        raise OutOfRangeError(
            f"machine {axis_name}={value}mm is outside travel "
            f"[{config.MACHINE_MIN_MM}, {config.MACHINE_MAX_MM}]mm"
        )


def local_to_machine_axes(orientation, x=None, y=None):
    """Map whichever table-local axes are supplied to their machine axes.

    Only axes actually passed in (not None) are mapped and returned, so
    callers can move X, Y, or both independently.

    Returns a dict such as {"x": mm} or {"x": mm, "y": mm}.
    """
    if orientation not in _AXIS_SWAP:
        raise ValueError(
            f"Unknown orientation {orientation!r}, expected one of {sorted(_AXIS_SWAP)}"
        )

    swapped = _AXIS_SWAP[orientation]
    result = {}
    if x is not None:
        axis, value = ("y", x) if swapped else ("x", x)
        _check_range(value, axis.upper())
        result[axis] = value
    if y is not None:
        axis, value = ("x", y) if swapped else ("y", y)
        _check_range(value, axis.upper())
        result[axis] = value
    return result


def orientation_angle(orientation):
    try:
        return config.ORIENTATION_ANGLES[orientation]
    except KeyError:
        raise ValueError(
            f"Unknown orientation {orientation!r}, expected one of {sorted(config.ORIENTATION_ANGLES)}"
        ) from None
