"""Example: transport a plate 1 -> 2 -> 3 -> 1 around the table sides.

Usage:
    python -m robot_client.examples.transport_sides_1_2_3_1 --port COM3
"""
import argparse

from robot_client.robot import RobotController

# Example pick/place coordinates, in table-local mm, within the 0-400 reach.
PICK_XY = (200.0, 100.0)
PLACE_XY = (150.0, 300.0)
Z_TRAVEL = 100.0
Z_PICK = 10.0
Z_PLACE = 10.0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=None, help="Serial port, e.g. COM3")
    args = parser.parse_args()

    with RobotController(port=args.port) as robot:
        robot.home()

        for from_side, to_side in [(1, 2), (2, 3), (3, 1)]:
            robot.transport(
                from_orientation=from_side,
                to_orientation=to_side,
                pick_xy=PICK_XY,
                place_xy=PLACE_XY,
                z_travel=Z_TRAVEL,
                z_pick=Z_PICK,
                z_place=Z_PLACE,
            )


if __name__ == "__main__":
    main()
