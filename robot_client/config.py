"""Robot and table geometry configuration."""

SERIAL_BAUD = 115200
DEFAULT_PORT = "COM5"

# Table geometry, in millimeters. Table module is 80 cm x 80 cm.
TABLE_SIDE_MM = 800.0

# Arm reach, in millimeters. 40 cm arm covers a quarter of one table module.
ARM_REACH_MM = 400.0

# Supported side orientations, clockwise from the home direction (+Y).
ORIENTATION_ANGLES = {
    1: 0.0,
    2: 90.0,
    3: 180.0,
    4: 270.0,
}
ROTATION_OFFSET_DEG = -5.0

DEFAULT_FEED_RATE_XY = 1500.0
DEFAULT_FEED_RATE_Z = 600.0
DEFAULT_FEED_RATE_A = 900.0
