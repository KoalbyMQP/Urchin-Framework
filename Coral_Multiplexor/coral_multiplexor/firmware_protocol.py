from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
import struct

from crab_client.types import CommandItem, TicketType, ValueKind


class FirmwareProtocolError(RuntimeError):
    pass


class PacketStreams(bytes, Enum):
    REQUEST = b"N"
    EXCHANGE = b"E"
    REACTION = b"R"
    DEBUG = b"D"


@dataclass(frozen=True)
class DecodedPacket:
    vpid: int
    stream: bytes
    payload: bytes


class FirmwareProtocol:
    _TICKET_STRUCT = struct.Struct("<I")
    _LOAD_HEADER = struct.Struct("<I50sBB")

    @staticmethod
    def encode_format_ticket(ticket_type: TicketType) -> bytes:
        return b"FormatTicket" + ticket_type.value.encode("utf-8")

    @staticmethod
    def encode_punch_ticket(ticket_id: int) -> bytes:
        return b"PunchTicket" + FirmwareProtocol._TICKET_STRUCT.pack(ticket_id)

    @staticmethod
    def encode_close_ticket(ticket_id: int) -> bytes:
        return b"CloseTicket" + FirmwareProtocol._TICKET_STRUCT.pack(ticket_id)

    @staticmethod
    def encode_ticket_info(ticket_id: int) -> bytes:
        return b"TicketInfo" + FirmwareProtocol._TICKET_STRUCT.pack(ticket_id)

    @staticmethod
    def encode_get_health(metric: str) -> bytes:
        return b"GetHealth" + metric.encode("utf-8")

    @staticmethod
    def encode_load_ticket(ticket_id: int, item: CommandItem) -> bytes:
        command_bytes = item.command.encode("utf-8")
        if len(command_bytes) > 255:
            raise FirmwareProtocolError("Command name exceeds 255 bytes")
        payload = bytearray()
        payload.extend(b"LoadTicket")
        payload.extend(
            FirmwareProtocol._LOAD_HEADER.pack(
                ticket_id,
                item.joint.encode("utf-8")[:50].ljust(50, b"\x00"),
                len(command_bytes),
                len(item.values),
            )
        )
        payload.extend(command_bytes)
        for value in item.values:
            if value.kind == ValueKind.INT:
                payload.extend(b"I" + struct.pack("<i", value.int_value))
            elif value.kind == ValueKind.FLOAT:
                payload.extend(b"F" + struct.pack("<f", value.float_value))
            elif value.kind == ValueKind.BOOL:
                payload.extend(b"B" + struct.pack("<B", int(value.bool_value)))
            else:
                raise FirmwareProtocolError(f"Unsupported value kind {value.kind}")
        return bytes(payload)

    @staticmethod
    def parse_exchange_int(payload: bytes) -> int:
        return struct.unpack("<i", payload[:4])[0]

    @staticmethod
    def parse_exchange_uint(payload: bytes) -> int:
        return struct.unpack("<I", payload[:4])[0]
