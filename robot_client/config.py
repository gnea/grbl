"""Robot and table geometry configuration."""

SERIAL_BAUD = 115200
DEFAULT_PORT = "COM5"

# Table geometry, in millimeters.
TABLE_SIDE_MM = 550.0

# Rod length from the Cartesian base to the gripper tip.
ARM_REACH_MM = 300.0

# Physical angular error at the A-axis home switch; the controller compensates once after homing.
HOMING_ROTATION_OFFSET_DEG = -5.0

# The simulator represents ideal geometry and therefore has no angular offset.
ROTATION_OFFSET_DEG = 0.0
A_AXIS_LIMIT_DEG = 360.0

DEFAULT_FEED_RATE_XY = 1500.0
DEFAULT_FEED_RATE_Z = 600.0
DEFAULT_FEED_RATE_A = 900.0
