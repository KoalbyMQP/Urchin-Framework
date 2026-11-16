"""Framed serial link to the Urchin firmware.

Every message in either direction is a fixed-size frame
(``Urchin_Firmware/src/ESP_PI_Communication/Shipping.c``)::

    0x07 | VPID (u8) | Stream (char) | data (1024 bytes, zero-padded)

The host always sends on stream ``N``. The firmware replies on ``E`` (exchange:
direct replies), ``R`` (reactions) and ``D`` (debug text).
"""

from __future__ import annotations

import logging
import os
import threading
import time
from collections.abc import Iterable
from dataclasses import dataclass
from typing import TYPE_CHECKING, Protocol

import serial
from serial.tools.list_ports import comports

if TYPE_CHECKING:
    from typing_extensions import Self

logger = logging.getLogger(__name__)

START_MARKER = b"\x07"
DATA_SIZE = 1024
FRAME_SIZE = 1 + 1 + DATA_SIZE  # VPID + Stream + data, after the start marker
HOST_STREAM = b"N"
BAUD_RATE = 115200

# D-BOX (Clam/D-BOX) frame, used when URCHIN_PROTOCOL=dbox:
#     0x07 | VPID (u8) | Stream (char) | payload size (u16 LE) | header CRC-16 (u16 LE) | payload
DBOX_HEADER_SIZE = 6  # VPID + Stream + size + CRC, after the start marker
DBOX_CRC_POLY = 0x9EB2  # same settings as Clam/D-BOX: init 0xFFFF, no reflection, no final XOR


def crc16(data: bytes) -> int:
    """D-BOX header checksum."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ DBOX_CRC_POLY) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def encode_dbox(vpid: int, payload: bytes) -> bytes:
    """Build a host-to-firmware D-BOX frame, including the start marker."""
    if not 0 <= vpid <= 255:
        raise LinkError(f"VPID must be between 0 and 255, got {vpid}")
    if len(payload) > 0xFFFF:
        raise LinkError(f"payload is {len(payload)} bytes; max is {0xFFFF}")
    header = bytes([vpid]) + HOST_STREAM + len(payload).to_bytes(2, "little")
    return START_MARKER + header + crc16(header).to_bytes(2, "little") + payload

VALIDATE_COMMAND = b"Validate"
BOARD_TAG = b"URCHIN"
USB_SERIAL_NUMBER = "urchin"

# USB-UART bridges and native USB found on ESP32 boards, probed before other ports
ESP32_USB_IDS = frozenset(
    {
        (0x10C4, 0xEA60),  # Silicon Labs CP210x
        (0x1A86, 0x7523),  # WCH CH340
        (0x1A86, 0x55D4),  # WCH CH9102
        (0x0403, 0x6001),  # FTDI FT232R
        (0x303A, 0x1001),  # Espressif native USB
    }
)


class Port(Protocol):
    """The subset of a pyserial port that ``Link`` uses."""

    def write(self, data: bytes, /) -> int | None: ...
    def read(self, size: int = 1, /) -> bytes: ...
    def read_until(self, expected: bytes = ..., size: int | None = ..., /) -> bytes: ...
    def close(self) -> None: ...


class LinkError(Exception):
    """Raised for framing errors or when no Urchin board can be reached."""


@dataclass(frozen=True, slots=True)
class Frame:
    vpid: int
    stream: str
    data: bytes


def encode_frame(vpid: int, payload: bytes) -> bytes:
    """Build a host-to-firmware frame, including the start marker."""
    if not 0 <= vpid <= 255:
        raise LinkError(f"VPID must be between 0 and 255, got {vpid}")
    if len(payload) > DATA_SIZE:
        raise LinkError(f"payload is {len(payload)} bytes; max is {DATA_SIZE}")
    return START_MARKER + bytes([vpid]) + HOST_STREAM + payload.ljust(DATA_SIZE, b"\0")


def decode_frame(raw: bytes) -> Frame:
    """Decode the bytes that follow a start marker."""
    if len(raw) != FRAME_SIZE:
        raise LinkError(f"expected {FRAME_SIZE} bytes, got {len(raw)}")
    return Frame(vpid=raw[0], stream=chr(raw[1]), data=raw[2:])


class Link:
    """A framed connection to the firmware over an open port."""

    def __init__(self, port: Port, protocol: str | None = None) -> None:
        self.port = port
        self.protocol = protocol or os.environ.get("URCHIN_PROTOCOL", "box")
        if self.protocol not in ("box", "dbox"):
            raise LinkError(f"unknown protocol {self.protocol!r}; use 'box' or 'dbox'")
        self._write_lock = threading.Lock()
        self._pending = bytearray()  # D-BOX: bytes read but not used yet

    @classmethod
    def open(cls, url: str | None = None) -> Self:
        """Connect to ``url``, else ``$URCHIN_PORT``, else the first board found by discovery.

        ``url`` may be a device path or any pyserial URL, e.g. ``socket://127.0.0.1:5555``
        for the QEMU simulator.
        """
        url = url or os.environ.get("URCHIN_PORT") or find_urchin_port()
        logger.info("Opening %s", url)
        try:
            port = serial.serial_for_url(url, baudrate=BAUD_RATE, timeout=1)
        except (serial.SerialException, OSError) as e:
            # pyserial wraps the OS error in a message that repeats the port name
            reason = e.__context__ if isinstance(e.__context__, OSError) else e
            raise LinkError(f"Could not open {url}: {reason}") from e
        return cls(port)

    def send(self, vpid: int, payload: bytes) -> None:
        frame = encode_dbox(vpid, payload) if self.protocol == "dbox" else encode_frame(vpid, payload)

        with self._write_lock:
            self.port.write(frame)
        if logger.isEnabledFor(logging.DEBUG):
            logger.debug("Sent VPID %d: %r", vpid, payload)

    def read(self) -> Frame | None:
        """Read the next frame, or return None if the port times out first."""
        if self.protocol == "dbox":
            return self._read_dbox()
        skipped = self.port.read_until(START_MARKER)
        if not skipped.endswith(START_MARKER):
            return None
        raw = self.port.read(FRAME_SIZE)
        if len(raw) < FRAME_SIZE:
            logger.warning("Incomplete frame: got %d of %d bytes", len(raw), FRAME_SIZE)
            return None
        frame = decode_frame(raw)
        if logger.isEnabledFor(logging.DEBUG):
            logger.debug(
                "Received VPID %d stream %s: %r",
                frame.vpid,
                frame.stream,
                frame.data.rstrip(b"\0"),
            )
        return frame

    def close(self) -> None:
        self.port.close()
    
    # --- D-BOX ---

    def _take(self, size: int) -> bytes:
        """Up to ``size`` bytes: leftover bytes first, then the port."""
        data = bytes(self._pending[:size])
        del self._pending[:size]
        if len(data) < size:
            data += self.port.read(size - len(data))
        return data

    def _find_marker(self) -> bool:
        """Skip to just after the next start marker; False if the port times out."""
        index = self._pending.find(START_MARKER)
        if index >= 0:
            del self._pending[: index + 1]
            return True
        self._pending.clear()
        return self.port.read_until(START_MARKER).endswith(START_MARKER)

    def _read_dbox(self) -> Frame | None:
        while self._find_marker():
            header = self._take(DBOX_HEADER_SIZE)
            if len(header) < DBOX_HEADER_SIZE:
                logger.warning("Incomplete D-BOX header: got %d bytes", len(header))
                return None
            if crc16(header[:4]) != int.from_bytes(header[4:6], "little"):
                # False start: a 0x07 that wasn't a frame. Search again from the next byte.
                self._pending[:0] = header
                continue
            size = int.from_bytes(header[2:4], "little")
            payload = self._take(size)
            if len(payload) < size:
                logger.warning("Incomplete D-BOX payload: got %d of %d bytes", len(payload), size)
                return None
            # Pad like the old fixed-size frame, so the rest of Crab sees the same data.
            frame = Frame(vpid=header[0], stream=chr(header[1]), data=payload.ljust(DATA_SIZE, b"\0"))
            if logger.isEnabledFor(logging.DEBUG):
                logger.debug("Received VPID %d stream %s: %r", frame.vpid, frame.stream, payload)
            return frame
        return None


    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc_info: object) -> None:
        self.close()


def order_ports(ports: Iterable[object]) -> list[str]:
    """Order port devices by how likely they are to be an Urchin board.

    Ports with the ``urchin`` USB serial number come first, then known ESP32 USB
    chips, then everything else; ties keep their original order.
    """

    def rank(port: object) -> int:
        serial_number = getattr(port, "serial_number", None) or ""
        if serial_number.lower() == USB_SERIAL_NUMBER:
            return 0
        if (getattr(port, "vid", None), getattr(port, "pid", None)) in ESP32_USB_IDS:
            return 1
        return 2

    return [port.device for port in sorted(ports, key=rank)]  # type: ignore[attr-defined]


def handshake(port: Port, attempts: int = 3, reply_timeout: float = 1.0) -> bool:
    """Send ``Validate`` and wait for the board's ``URCHIN`` reply on the debug stream."""
    link = Link(port)
    for _ in range(attempts):
        link.send(0, VALIDATE_COMMAND)
        deadline = time.monotonic() + reply_timeout
        while time.monotonic() < deadline:
            frame = link.read()
            if (
                frame
                and frame.vpid == 0
                and frame.stream == "D"
                and frame.data.startswith(BOARD_TAG)
            ):
                return True
    return False


def probe(device: str, settle: float = 0.5) -> bool:
    """Return True if an Urchin board answers on ``device``."""
    logger.debug("Probing %s", device)
    try:
        port = serial.Serial(device, BAUD_RATE, timeout=0.1, dsrdtr=False, rtscts=False)
    except (serial.SerialException, OSError) as e:
        logger.debug("Cannot open %s: %s", device, e)
        return False
    try:
        # Opening the port can reset the ESP32; let it boot, then drop the boot output
        time.sleep(settle)
        port.reset_input_buffer()
        port.reset_output_buffer()
        return handshake(port)
    except (serial.SerialException, OSError) as e:
        logger.debug("Probe of %s failed: %s", device, e)
        return False
    finally:
        port.close()


def find_urchin_port() -> str:
    """Return the device path of the first port with an Urchin board."""
    devices = order_ports(comports())
    for device in devices:
        if probe(device):
            logger.info("Found Urchin board on %s", device)
            return device
    raise LinkError(
        f"No Urchin board found on {len(devices)} port(s). Check the USB connection, "
        f"give the board's USB-UART the serial number '{USB_SERIAL_NUMBER}', or set URCHIN_PORT."
    )
