from __future__ import annotations

import struct
from typing import Any

try:
    import serial
    import serial.tools.list_ports
except ImportError:  # pragma: no cover
    serial = None


class SerialTransportError(ConnectionError):
    pass


class SerialTransport:
    START_MARKER = b"\x07"
    PACKET_FORMAT = "<Bc1024s"
    PACKET_STRUCT = struct.Struct(PACKET_FORMAT)

    def __init__(self, debug: bool = False, port_name: str | None = None) -> None:
        if serial is None:
            raise SerialTransportError("pyserial is not installed")
        self.debug = debug
        self._port_name = port_name or self.find_esp32_port()
        self._bus = serial.Serial(self._port_name, 115200, timeout=1)

    @staticmethod
    def find_esp32_port() -> str:
        if serial is None:
            raise SerialTransportError("pyserial is not installed")
        for port in serial.tools.list_ports.comports():
            if port.serial_number and "urchin" in port.serial_number.lower():
                return port.device
        raise SerialTransportError('No Urchin serial port found; expected serial number containing "urchin"')

    def send_packet(self, vpid: int, stream: bytes, payload: bytes) -> None:
        if len(stream) != 1:
            raise ValueError("stream must be a single byte")
        packed = self.PACKET_STRUCT.pack(vpid, stream, payload.ljust(1024, b"\x00")[:1024])
        self._bus.write(self.START_MARKER + packed)

    def read_packet(self) -> dict[str, Any] | None:
        if not self._find_start_marker():
            return None
        raw_packet = self._bus.read(self.PACKET_STRUCT.size)
        if len(raw_packet) != self.PACKET_STRUCT.size:
            return None
        vpid, stream, payload = self.PACKET_STRUCT.unpack(raw_packet)
        return {"VPID": vpid, "Stream": stream, "data": payload}

    def _find_start_marker(self) -> bool:
        while True:
            marker = self._bus.read(1)
            if not marker:
                return False
            if marker == self.START_MARKER:
                return True

    def close(self) -> None:
        self._bus.close()
