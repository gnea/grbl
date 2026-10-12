"""Example: transport a plate 1 -> 2 -> 3 -> 4 -> 1 around the table sides.

Usage:
    python -m robot_client.examples.transport_sides_1_2_3_1 --port COM3
"""
import argparse
import sys
from pathlib import Path

if __package__:
    from robot_client.robot import RobotController
else:
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
    from robot_client.robot import RobotController

# Example pick/place coordinates, in table-local mm, within the 0-300 reach.
PICK_XYZ = (200.0, 0.0, 30.0)
PLACE_XYZ = (150.0, 0.0, 30.0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=None, help="Serial port, e.g. COM3")
    args = parser.parse_args()

    with RobotController(port=args.port) as robot:
        for from_side, to_side in [(1, 2), (2, 3), (3, 4), (4, 1)]:
            robot.transport(
                from_orientation=from_side,
                to_orientation=to_side,
                pick_xyz=PICK_XYZ,
                place_xyz=PLACE_XYZ,
            )


if __name__ == "__main__":
    main()
