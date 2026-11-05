"""Demo: the old Crab_API main.py program on the new API.

Swings the Shoulder joint back and forth and runs an AckCheck health check each cycle.

    uv run python examples/demo.py                 # discovery, or $URCHIN_PORT
    uv run python examples/demo.py --url socket://127.0.0.1:5555 -v
"""

import argparse
import logging
import time
from pathlib import Path

from crab_api2 import herkulex
from crab_api2.crab import Crab, MotorStatus, TicketType
from crab_api2.herkulex import JogLedColor
from crab_api2.log import setup_logging

DEFAULT_BRIDGE = Path(__file__).with_name("bridge.toml")


def print_ack_check(statuses: dict[str, MotorStatus]) -> None:
    for joint, status in statuses.items():
        state = "ok" if status.responding else "NOT RESPONDING"
        print(
            f"  {joint} (motor {status.number}): {state}, "
            f"detail=0x{status.status_detail:02X} error=0x{status.status_error:02X}"
        )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bridge", type=Path, default=DEFAULT_BRIDGE)
    parser.add_argument(
        "--url", help="device or pyserial URL (default: $URCHIN_PORT, then discovery)"
    )
    parser.add_argument(
        "--cycles", type=int, default=0, help="number of cycles (default: forever)"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="show debug logs")
    args = parser.parse_args()

    setup_logging(logging.DEBUG if args.verbose else logging.INFO)

    with Crab(args.bridge, args.url) as crab:
        time.sleep(3)
        cycle = 0
        try:
            while args.cycles == 0 or cycle < args.cycles:
                cycle += 1
                print(f"Cycle {cycle}: AckCheck")
                print_ack_check(crab.get_health("AckCheck"))

                time.sleep(3)
                command = herkulex.move_one("Shoulder", -23.0, 40, JogLedColor.LED_BLUE)
                print(
                    f"Move Shoulder to -23: ticket {crab.send(TicketType.ASYNCHRONOUS, [command])}"
                )

                time.sleep(1)
                command = herkulex.move_one("Shoulder", 23.0, 40, JogLedColor.LED_RED)
                print(
                    f"Move Shoulder to 23: ticket {crab.send(TicketType.ASYNCHRONOUS, [command])}"
                )

                time.sleep(1)
                print(f"Cycle {cycle}: AckCheck")
                print_ack_check(crab.get_health("AckCheck"))
                time.sleep(1)
        except KeyboardInterrupt:
            print("Stopping")


if __name__ == "__main__":
    main()
