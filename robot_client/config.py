"""Robot and table geometry configuration."""

SERIAL_BAUD = 115200
DEFAULT_PORT = "COM5"

# Table geometry, in millimeters.
TABLE_SIDE_MM = 800.0

PLACEMENT_OFFSET_SIDE1_MM = 155.0
PLACEMENT_OFFSET_SIDE2_MM = 160.0
PLACEMENT_OFFSET_SIDE3_MM = 95.0
PLACEMENT_OFFSET_SIDE4_MM = 110.0

# Distances from the safe base-travel box to each table edge (side 1 = +X, 2 = +Y, 3 = -X, 4 = -Y).
# They fix the box size and position: machine (0, 0) sits at table (SIDE3, SIDE4).
SAFE_TRAVEL_X_MM = TABLE_SIDE_MM - PLACEMENT_OFFSET_SIDE1_MM - PLACEMENT_OFFSET_SIDE3_MM
SAFE_TRAVEL_Y_MM = TABLE_SIDE_MM - PLACEMENT_OFFSET_SIDE2_MM - PLACEMENT_OFFSET_SIDE4_MM

# Rod length from the Cartesian base to the gripper tip.
ARM_REACH_MM = 265.0

# Physical angular error at the A-axis home switch; the controller compensates once after homing.
HOMING_ROTATION_OFFSET_DEG = -5.0

# The simulator represents ideal geometry and therefore has no angular offset. This should be fine-tuned based on real-world measurements.
ROTATION_OFFSET_DEG = 0.0
A_AXIS_LIMIT_DEG = 270.0

DEFAULT_FEED_RATE_XY = 6000
DEFAULT_FEED_RATE_Z = 2000
DEFAULT_FEED_RATE_A = 1200