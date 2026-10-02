"""Command-line interface for the wellplate transport robot client.

Examples:
    python -m robot_client.cli --port COM3 home
    python -m robot_client.cli --port COM3 orient 2
    python -m robot_client.cli --port COM3 move 1 --x 200
    python -m robot_client.cli --port COM3 move 1 --x 200 --y 100 --z 20
    python -m robot_client.cli --port COM3 transport --from-side 1 --to-side 2 \\
        --pick-x 200 --pick-y 100 --place-x 150 --place-y 300
"""
import sys

from . import config
from .robot import RobotController

try:
    import argparse
except ImportError:  # pragma: no cover - argparse is stdlib
    raise


def build_parser():
    parser = argparse.ArgumentParser(description="Grbl wellplate robot client")
    parser.add_argument("--port", default=config.DEFAULT_PORT, help="Serial port, e.g. COM3")
    parser.add_argument("--baud", type=int, default=config.SERIAL_BAUD)
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("home", help="Run the Grbl homing cycle")

    orient = sub.add_parser("orient", help="Move to mid-reach and rotate to a table side")
    orient.add_argument("side", type=int, choices=sorted(config.ORIENTATION_ANGLES))

    move = sub.add_parser("move", help="Move to a table-local position for a given side")
    move.add_argument("side", type=int, choices=sorted(config.ORIENTATION_ANGLES))
    move.add_argument("--x", type=float, default=None)
    move.add_argument("--y", type=float, default=None)
    move.add_argument("--z", type=float, default=None)

    transport = sub.add_parser("transport", help="Pick a plate from one side and place it on another")
    transport.add_argument("--from-side", type=int, required=True, dest="from_side",
                            choices=sorted(config.ORIENTATION_ANGLES))
    transport.add_argument("--to-side", type=int, required=True, dest="to_side",
                            choices=sorted(config.ORIENTATION_ANGLES))
    transport.add_argument("--pick-x", type=float, required=True)
    transport.add_argument("--pick-y", type=float, required=True)
    transport.add_argument("--place-x", type=float, required=True)
    transport.add_argument("--place-y", type=float, required=True)
    transport.add_argument("--z-travel", type=float, default=100.0)
    transport.add_argument("--z-pick", type=float, default=10.0)
    transport.add_argument("--z-place", type=float, default=10.0)

    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)

    with RobotController(port=args.port, baud=args.baud) as robot:
        if args.command == "home":
            robot.home()
        elif args.command == "orient":
            robot.set_orientation(args.side)
        elif args.command == "move":
            robot.set_orientation(args.side)
            robot.move_to(x=args.x, y=args.y, z=args.z)
        elif args.command == "transport":
            robot.transport(
                from_orientation=args.from_side,
                to_orientation=args.to_side,
                pick_xy=(args.pick_x, args.pick_y),
                place_xy=(args.place_x, args.place_y),
                z_travel=args.z_travel,
                z_pick=args.z_pick,
                z_place=args.z_place,
            )


if __name__ == "__main__":
    sys.exit(main())
