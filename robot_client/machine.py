"""Derive safe axis bounds from GRBL-CRPT settings."""
from typing import Dict, Tuple

from . import config
from .crpt_link import CrptError

AxisBounds = Dict[str, Tuple[float, float]]
MachineSettings = Dict[int, float]

_REQUIRED_SETTINGS = (28, 130, 131, 132, 133, 140, 141, 142, 143)
# (axis, homing-direction mask bit, travel setting, pull-off setting)
_LINEAR_AXES = (("x", 1, 130, 140), ("y", 2, 131, 141), ("z", 4, 132, 142))


def compute_axis_bounds(settings: MachineSettings) -> AxisBounds:
    missing = [n for n in _REQUIRED_SETTINGS if n not in settings]
    if missing:
        raise CrptError(
            "Firmware is missing robot travel/pull-off settings: "
            + ", ".join(f"${n}" for n in missing)
            + ". Flash the updated robot firmware first."
        )

    maximum_end_mask = int(settings[28])
    bounds: AxisBounds = {}
    for axis, bit, travel_setting, pull_off_setting in _LINEAR_AXES:
        travel = settings[travel_setting]
        pull_off = settings[pull_off_setting]
        if travel <= 0 or pull_off < 0 or pull_off >= travel:
            raise CrptError(f"Invalid travel/pull-off configuration for {axis.upper()}")
        bounds[axis] = (0.0, travel - pull_off) if maximum_end_mask & bit else (pull_off, travel)

    a_travel = settings[133]
    if a_travel <= 0:
        raise CrptError("Invalid A-axis travel setting $133")
    bounds["a"] = (0.0, min(a_travel, config.A_AXIS_LIMIT_DEG))
    return bounds
