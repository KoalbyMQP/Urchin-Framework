"""A scripted fake of the Urchin firmware, shared by the Crab and CLI tests.

The fake answers the way Urchin_Firmware/src/Conversation/UnPacker.cpp does:
FormatTicket -> E: i32 error [, u32 ticket]; LoadTicket TestReact -> R reaction;
GetHealth FreeRam/TotalRam -> E text; GetHealth AckCheck -> D lines.
"""

import struct
import threading

from crab_api2.link import DATA_SIZE, START_MARKER


def frame(stream: str, data: bytes) -> bytes:
    return START_MARKER + b"\x00" + stream.encode() + data.ljust(DATA_SIZE, b"\0")


def reaction_data(ticket: int, joint: bytes, value: int, code: int = 45) -> bytes:
    command = b"TestReact"
    return (
        struct.pack("<II", ticket, code)
        + joint.ljust(50, b"\0")
        + bytes([len(command), 1])
        + command
        + b"i"
        + struct.pack("<i", value)
    )


class FakeFirmware:
    """A pyserial-like port backed by a scripted firmware."""

    timeout = 0.02

    def __init__(self) -> None:
        self.payloads: list[bytes] = []
        self.next_ticket = 0
        self.format_error = 0
        self.silent = False
        self.ack_lines: list[bytes] = []
        self.closed = False
        self._buffer = bytearray()
        self._cv = threading.Condition()

    # --- scripting ---

    def inject(self, data: bytes) -> None:
        with self._cv:
            self._buffer += data
            self._cv.notify_all()

    def respond(self, payload: bytes) -> bytes:
        if self.silent:
            return b""
        if payload.startswith(b"FormatTicket"):
            if self.format_error:
                return frame("E", struct.pack("<i", self.format_error))
            ticket = self.next_ticket
            self.next_ticket += 1
            return frame("E", struct.pack("<i", 0)) + frame(
                "E", struct.pack("<I", ticket)
            )
        if payload.startswith(b"LoadTicket"):
            ticket = struct.unpack_from("<I", payload, 10)[0]
            joint = payload[14:64].rstrip(b"\0")
            name_len = payload[64]
            name = payload[66 : 66 + name_len]
            if name == b"TestReact":
                value = struct.unpack_from("<i", payload, 66 + name_len + 1)[0]
                return frame("R", reaction_data(ticket, joint, value))
            return frame("D", b"LoadTicket called")
        if payload.startswith(b"GetHealthFreeRam\0"):
            return frame("E", b"123456\0")
        if payload.startswith(b"GetHealthTotalRam\0"):
            return frame("E", b"98765\0")
        if payload.startswith(b"GetHealthAckCheck\0"):
            lines = [b"GetHealth:AckCheck", *self.ack_lines, b"AckCheck:done"]
            return b"".join(frame("D", line + b"\0") for line in lines)
        return b""

    def sent(self, prefix: bytes) -> list[bytes]:
        return [p for p in self.payloads if p.startswith(prefix)]

    # --- pyserial surface used by Link ---

    def write(self, data: bytes) -> int:
        assert data[:1] == START_MARKER and len(data) == 3 + DATA_SIZE
        payload = bytes(data[3:])
        self.payloads.append(payload)
        self.inject(self.respond(payload))
        return len(data)

    def read_until(self, expected: bytes = b"\n", size: int | None = None) -> bytes:
        with self._cv:
            self._cv.wait_for(lambda: expected in self._buffer, timeout=self.timeout)
            index = self._buffer.find(expected)
            end = len(self._buffer) if index == -1 else index + len(expected)
            chunk = bytes(self._buffer[:end])
            del self._buffer[:end]
            return chunk

    def read(self, size: int = 1) -> bytes:
        with self._cv:
            self._cv.wait_for(lambda: len(self._buffer) >= size, timeout=self.timeout)
            chunk = bytes(self._buffer[:size])
            del self._buffer[:size]
            return chunk

    def close(self) -> None:
        self.closed = True
