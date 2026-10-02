"""Robot and table geometry configuration."""

SERIAL_BAUD = 115200
DEFAULT_PORT = "COM3"

# Table geometry, in millimeters. Table module is 80 cm x 80 cm.
TABLE_SIDE_MM = 800.0

# Arm reach, in millimeters. 40 cm arm covers a quarter of one table module.
ARM_REACH_MM = 400.0

# Grbl machine travel limits for X and Y ($130 / $131), in millimeters.
MACHINE_MIN_MM = 0.0
MACHINE_MAX_MM = ARM_REACH_MM

# Supported table-side orientations. The A axis rotates the gripper tip
# only; the X/Y gantry itself does not move. Rotating 90 degrees means the
# local X and Y table axes swap which physical machine axis they drive; a
# further 90 degrees (180 total) swaps back to the unswapped mapping.
ORIENTATION_ANGLES = {
    1: 0.0,
    2: 90.0,
    3: 180.0,
}

# X/Y position the arm parks at before rotating orientation.
ORIENTATION_CHANGE_X = ARM_REACH_MM / 2.0
ORIENTATION_CHANGE_Y = ARM_REACH_MM / 2.0

DEFAULT_FEED_RATE_XY = 1500.0
DEFAULT_FEED_RATE_Z = 600.0
DEFAULT_FEED_RATE_A = 900.0
