"""
Pure-Python implementation of Gabe's D-BOX framing (Clam/D-BOX), so Python
tools can talk D-BOX with nothing more than pyserial -- no CMake/Boost/
pybind11 build of Clam's DBOXpy bindings needed.

Wire format (must stay byte-identical to Clam/D-BOX/include/DBOXprotocol.h,
DBOXraw::operator std::vector<uint8_t>):

    [0x07] [VPID] [Stream] [len lo] [len hi] [crc lo] [crc hi] [payload ...]

    - 0x07 (bell, '\\a') marks the start of a frame
    - len: payload length, little-endian uint16
    - crc: CRC-16 over the 4 header bytes (VPID, Stream, len lo, len hi),
      little-endian. Polynomial 0x9EB2, init 0xFFFF, no reflection, no
      final XOR -- boost::crc_optimal<16, 0x9eb2, 0xFFFF, 0, false, false>
      in Clam's DBOXprotocol.cpp. The payload itself is not checksummed.

Byte-compatibility with the C++ implementation is checked end-to-end by
bench_test.py's link test against the ESP32 running env:DBoxEcho (which
echoes frames back using Clam's own C++ code).
"""

import time
from typing import NamedTuple, Optional, Union

import serial

START_MARKER = 0x07
GENERATOR_POLYNOMIAL = 0x9EB2
HEADER_SIZE = 4   # VPID, Stream, len lo, len hi
FRAME_OVERHEAD = 1 + HEADER_SIZE + 2   # start marker + header + CRC
MAX_PAYLOAD = 0xFFFF


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ GENERATOR_POLYNOMIAL) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


class Packet(NamedTuple):
    vpid: int
    stream: str      # single character, e.g. 'N', 'E', 'D', 'S'
    payload: bytes

    @property
    def text(self) -> str:
        return self.payload.decode("utf-8", errors="replace").rstrip("\x00")


def encode(vpid: int, stream: str, payload: Union[bytes, str]) -> bytes:
    if isinstance(payload, str):
        payload = payload.encode("utf-8")
    if not 0 <= vpid <= 255:
        raise ValueError("VPID must be 0-255")
    if len(stream) != 1:
        raise ValueError("stream must be a single character")
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too large for D-BOX (max 65535 bytes)")

    size = len(payload)
    header = bytes([vpid, ord(stream), size & 0xFF, (size >> 8) & 0xFF])
    crc = crc16(header)
    return bytes([START_MARKER]) + header + bytes([crc & 0xFF, (crc >> 8) & 0xFF]) + payload


class Decoder:
    """
    Incremental frame decoder: feed it whatever bytes arrived, pull out
    complete frames. Anything that isn't a valid frame (ESP32 boot text,
    line noise, a 0x07 that wasn't really a start marker) is skipped, and a
    header that fails its CRC only costs that one start byte -- decoding
    resumes at the next 0x07.
    """

    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> None:
        self._buffer.extend(data)

    def next_packet(self) -> Optional[Packet]:
        while True:
            start = self._buffer.find(bytes([START_MARKER]))
            if start < 0:
                self._buffer.clear()
                return None
            del self._buffer[:start]

            if len(self._buffer) < FRAME_OVERHEAD:
                return None   # header not fully arrived yet

            header = bytes(self._buffer[1:1 + HEADER_SIZE])
            received_crc = self._buffer[5] | (self._buffer[6] << 8)
            if crc16(header) != received_crc:
                del self._buffer[:1]   # false start; resync on the next 0x07
                continue

            size = header[2] | (header[3] << 8)
            if len(self._buffer) < FRAME_OVERHEAD + size:
                return None   # payload not fully arrived yet

            payload = bytes(self._buffer[FRAME_OVERHEAD:FRAME_OVERHEAD + size])
            del self._buffer[:FRAME_OVERHEAD + size]
            return Packet(header[0], chr(header[1]), payload)


class DBoxLink:
    """A D-BOX connection over a serial port."""

    def __init__(self, port: str, baud: int = 115200) -> None:
        # Same open settings Crab_API's ESPSerial uses with the ESP32.
        # serial_for_url also accepts plain port names ("COM5",
        # "/dev/ttyUSB0"), plus URLs like socket://host:port for tests.
        self.serial = serial.serial_for_url(port, baud, timeout=0.05, dsrdtr=False, rtscts=False)
        self._decoder = Decoder()

    def close(self) -> None:
        self.serial.close()

    def discard_input(self) -> None:
        self.serial.reset_input_buffer()
        self._decoder = Decoder()

    def send(self, vpid: int, stream: str, payload: Union[bytes, str]) -> None:
        self.serial.write(encode(vpid, stream, payload))

    def receive(self, timeout: float) -> Optional[Packet]:
        """Next complete frame, or None if none arrives within timeout seconds."""
        deadline = time.monotonic() + timeout
        while True:
            packet = self._decoder.next_packet()
            if packet is not None:
                return packet
            if time.monotonic() >= deadline:
                return None
            data = self.serial.read(self.serial.in_waiting or 1)
            if data:
                self._decoder.feed(data)
