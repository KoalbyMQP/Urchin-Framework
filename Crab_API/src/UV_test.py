"""
UV-Test: basic Urchin Framework communication test.

Purpose:
1. List serial ports visible to the computer.
2. Use the existing Crab_API ESPSerial class.
3. Find an ESP32 running Urchin firmware.
4. Send the "Validate" RFC.
5. Confirm that the ESP32 replies with "URCHIN".

This test does NOT require a motor.
The ESP32 must already be flashed with compatible Urchin firmware.

Recommended location in the repo:
    Crab_API/src/UV_test.py

Run from Crab_API/src:
    python UV_test.py
"""

import time
import serial.tools.list_ports

from ESPSerial import ESPSerial


VPID = 0
VALIDATE_COMMAND = b"Validate"
EXPECTED_REPLY = b"URCHIN"


def list_serial_ports() -> None:
    """Print the serial devices currently visible to the computer."""
    print("\n=== Serial ports detected ===")

    ports = list(serial.tools.list_ports.comports())

    if not ports:
        print("No serial ports found.")
        return

    for port in ports:
        print(f"Device       : {port.device}")
        print(f"Description  : {port.description}")
        print(f"Manufacturer : {port.manufacturer}")
        print(f"Serial No.   : {port.serial_number}")
        print()


def validate_once(esp: ESPSerial) -> bool:
    """Send one Validate request and check for the expected Urchin response."""
    print("Sending Validate request...")

    start_time = time.perf_counter()

    esp.send_packet(VPID, VALIDATE_COMMAND)
    packet = esp.read_packet()

    elapsed_ms = (time.perf_counter() - start_time) * 1000

    if packet is None:
        print(f"FAIL: No packet received. Round-trip attempt: {elapsed_ms:.2f} ms")
        return False

    reply = packet["data"].rstrip(b"\x00")
    stream = packet["Stream"]
    returned_vpid = packet["VPID"]

    print(f"Returned VPID : {returned_vpid}")
    print(f"Stream        : {stream}")
    print(f"Payload       : {reply!r}")
    print(f"Round-trip    : {elapsed_ms:.2f} ms")

    valid = (
        returned_vpid == VPID
        and stream == b"D"
        and reply.startswith(EXPECTED_REPLY)
    )

    if valid:
        print("PASS: Urchin firmware responded correctly.")
    else:
        print("FAIL: A packet arrived, but it was not the expected Validate response.")

    return valid


def main() -> None:
    print("====================================")
    print(" Urchin Framework Basic Test")
    print("====================================")

    list_serial_ports()

    print("Attempting to find an Urchin ESP32...")
    print("ESPSerial will probe available ports using the existing Validate mechanism.\n")

    esp = None

    try:
        esp = ESPSerial(Debug=True)

        print("\nConnected to an Urchin-compatible ESP32.")
        print(f"Opened serial port: {esp.buss.port}\n")

        validate_once(esp)

    except ConnectionError as exc:
        print("\nCONNECTION FAILED")
        print(exc)
        print("\nCheck that:")
        print("  1. The ESP32 is connected with a data-capable USB cable.")
        print("  2. Urchin firmware has been successfully flashed.")
        print("  3. No other program currently has the serial port open.")

    except Exception as exc:
        print(f"\nUnexpected error: {type(exc).__name__}: {exc}")

    finally:
        if esp is not None:
            esp.close()
            print("\nSerial connection closed.")


if __name__ == "__main__":
    main()
