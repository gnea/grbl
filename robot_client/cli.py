"""Command-line interface for the Cartesian Robot Plate Transporter (CRPT).

Examples:
    python -m robot_client.cli --port COM3 home
    python -m robot_client.cli --port COM3 orient 2
    python -m robot_client.cli --port COM3 move 1 --x 200
    python -m robot_client.cli --port COM3 move 1 --x 200 --y 100 --z 20
    python -m robot_client.cli --port COM3 transport --from-side 1 --to-side 2 \\
        --pick-x 200 --pick-y 100 --pick-z 10 --place-x 150 --place-y 300 --place-z 10
    python -m robot_client.cli --sim transport --from-side 1 --to-side 2 \\
        --pick-x 200 --pick-y 0 --place-x 30 --place-y 200
"""
import argparse
import sys

from . import config
from .coordinates import SIDES
from .robot import RobotController


def build_parser():
    parser = argparse.ArgumentParser(description="Cartesian Robot Plate Transporter (CRPT)")
    parser.add_argument("--port", default=config.DEFAULT_PORT, help="Serial port, e.g. COM3")
    parser.add_argument("--baud", type=int, default=config.SERIAL_BAUD)
    parser.add_argument("--sim", action="store_true", help="Use the local browser simulator instead of serial")
    parser.add_argument("--simulator-url", default=None, help="Attach to a running simulator URL")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("home", help="Run the GRBL-CRPT homing cycle")

    orient = sub.add_parser("orient", help="Raise Z and face a table side")
    orient.add_argument("side", type=int, choices=sorted(SIDES))

    move = sub.add_parser("move", help="Move to a table-local position for a given side")
    move.add_argument("side", type=int, choices=sorted(SIDES))
    move.add_argument("--x", type=float, default=None)
    move.add_argument("--y", type=float, default=None)
    move.add_argument("--z", type=float, default=None)

    transport = sub.add_parser("transport", help="Pick a plate from one side and place it on another")
    transport.add_argument("--from-side", type=int, required=True, dest="from_side",
                            choices=sorted(SIDES))
    transport.add_argument("--to-side", type=int, required=True, dest="to_side",
                            choices=sorted(SIDES))
    transport.add_argument("--pick-x", type=float, required=True)
    transport.add_argument("--pick-y", type=float, required=True)
    transport.add_argument("--pick-z", type=float, default=10.0)
    transport.add_argument("--place-x", type=float, required=True)
    transport.add_argument("--place-y", type=float, required=True)
    transport.add_argument("--place-z", type=float, default=10.0)

    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)

    with RobotController(
        port=args.port,
        baud=args.baud,
        sim=args.sim,
        simulator_url=args.simulator_url,
    ) as robot:
        if args.command == "home":
            return
        elif args.command == "orient":
            robot.set_orientation(args.side)
        elif args.command == "move":
            robot.set_orientation(args.side)
            robot.move_to(x=args.x, y=args.y, z=args.z)
        elif args.command == "transport":
            robot.transport(
                from_orientation=args.from_side,
                to_orientation=args.to_side,
                pick_xyz=(args.pick_x, args.pick_y, args.pick_z),
                place_xyz=(args.place_x, args.place_y, args.place_z),
            )


if __name__ == "__main__":
    sys.exit(main())
