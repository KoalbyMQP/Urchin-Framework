"""The ``crab-api2`` command.

crab-api2 ports                         list serial ports; mark the ones with an Urchin board
crab-api2 check BRIDGE [--url URL]      connect, upload the bridge, and run health checks
"""

from __future__ import annotations

import argparse
import logging
import os
import sys
from collections.abc import Sequence

from crab_api2 import link
from crab_api2.bridge import BridgeError
from crab_api2.crab import Crab, CrabError, MotorStatus
from crab_api2.link import LinkError
from crab_api2.log import setup_logging


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="crab-api2", description="Crab API tools")
    parser.add_argument("-v", "--verbose", action="store_true", help="show debug logs")
    commands = parser.add_subparsers(dest="command", required=True)

    commands.add_parser(
        "ports", help="list serial ports and probe them for an Urchin board"
    )

    check = commands.add_parser(
        "check", help="connect, upload a bridge, and run health checks"
    )
    check.add_argument("bridge", help="bridge .toml file")
    check.add_argument(
        "--url", help="device or pyserial URL (default: $URCHIN_PORT, then discovery)"
    )

    args = parser.parse_args(argv)
    setup_logging(logging.DEBUG if args.verbose else logging.INFO)

    if args.command == "ports":
        return _ports()
    return _check(args.bridge, args.url)


def _ports() -> int:
    url = os.environ.get("URCHIN_PORT")
    if url:
        print(f"URCHIN_PORT is set: {url} (used instead of these ports)")

    ports = {port.device: port for port in link.comports()}
    if not ports:
        print("No serial ports found.")
        return 0

    for device in link.order_ports(ports.values()):
        port = ports[device]
        ids = []
        if port.vid is not None and port.pid is not None:
            ids.append(f"{port.vid:04X}:{port.pid:04X}")
        if port.serial_number:
            ids.append(f"sn={port.serial_number}")
        status = "Urchin board" if link.probe(device) else "no reply"
        print(f"{device}  {port.description}  [{' '.join(ids) or '-'}]  {status}")
    return 0


def _check(bridge: str, url: str | None) -> int:
    try:
        with Crab(bridge, url) as crab:
            print(f"Bridge uploaded: {len(crab.motors)} motors")
            print(f"FreeRam: {crab.get_health('FreeRam')} bytes")
            statuses = crab.get_health("AckCheck")
            print("AckCheck:")
            healthy = True
            for motor in crab.motors:
                status = statuses.get(motor.joint)
                print(f"  {motor.joint} (motor {motor.number}): {_describe(status)}")
                healthy &= status is not None and status.responding
    except (BridgeError, CrabError, LinkError) as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    return 0 if healthy else 1


def _describe(status: MotorStatus | None) -> str:
    if status is None:
        return "NO REPORT"
    if not status.responding:
        return "NOT RESPONDING"
    return f"ok, detail=0x{status.status_detail:02X} error=0x{status.status_error:02X}"
