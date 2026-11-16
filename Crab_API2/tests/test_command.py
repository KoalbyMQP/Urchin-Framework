"""Checks of the LoadTicket packet against the firmware layout.

Urchin_Firmware/src/Conversation/UnPacker.cpp::LoadTicket (packed, little-endian):
    PacketHeader:
        unsigned int ticket;       4
        char joint[50];           50
        unsigned char command_len; 1
        unsigned char values_len;  1
    char command[command_len]
    Data[values_len]:
        char Type;                 1
        union { int; float; char } 4   -> 5 bytes per value

Receving.cpp dispatches on the "LoadTicket" prefix and passes the rest to LoadTicket.
"""

import struct
from enum import IntEnum

import pytest

from crab_api2.command import Command, encode_value


def i32(value: int) -> bytes:
    return value.to_bytes(4, "little", signed=True)


def f32(value: float) -> bytes:
    return struct.pack("<f", value)


# --- value encoding ---


@pytest.mark.parametrize(
    ("value", "expected"),
    [
        (0, b"I" + i32(0)),
        (-23, b"I" + i32(-23)),
        (2**31 - 1, b"I" + i32(2**31 - 1)),
        (-(2**31), b"I" + i32(-(2**31))),
        (23.5, b"F" + f32(23.5)),
        (-0.25, b"F" + f32(-0.25)),
        (True, b"B\x01\x00\x00\x00"),
        (False, b"B\x00\x00\x00\x00"),
    ],
)
def test_encode_value(value, expected):
    encoded = encode_value(value)
    assert encoded == expected
    assert len(encoded) == 5


def test_encode_int_enum_as_int():
    class Color(IntEnum):
        BLUE = 3

    assert encode_value(Color.BLUE) == b"I" + i32(3)


@pytest.mark.parametrize("value", [2**31, -(2**31) - 1])
def test_int_out_of_range(value):
    with pytest.raises(ValueError, match="32-bit"):
        encode_value(value)


def test_float_out_of_range():
    with pytest.raises(ValueError, match="32-bit float"):
        encode_value(1e39)


@pytest.mark.parametrize("value", ["7", None, b"x", [1]])
def test_unsupported_value_type(value):
    with pytest.raises(TypeError, match="must be int, float, or bool"):
        encode_value(value)


# --- Command validation ---


def test_command_fields():
    command = Command("Shoulder", "MoveOne", [23.0, 40, 3])
    assert command.joint == "Shoulder"
    assert command.name == "MoveOne"
    assert command.values == (23.0, 40, 3)


def test_command_default_no_values():
    assert Command("Shoulder", "GetAngle").values == ()


def test_command_is_immutable():
    command = Command("Shoulder", "GetAngle")
    with pytest.raises(AttributeError):
        command.joint = "Hip"  # ty: ignore


def test_empty_joint_rejected():
    with pytest.raises(ValueError, match="joint must not be empty"):
        Command("", "SetTorque", [1])


def test_joint_too_long():
    Command("j" * 49, "GetAngle")
    with pytest.raises(ValueError, match="joint must be at most 49 bytes"):
        Command("j" * 50, "GetAngle")


def test_empty_name_rejected():
    with pytest.raises(ValueError, match="name must not be empty"):
        Command("Shoulder", "")


def test_name_too_long():
    Command("Shoulder", "n" * 254)
    with pytest.raises(ValueError, match="name must be at most 254 bytes"):
        Command("Shoulder", "n" * 255)


def test_too_many_values():
    Command("Shoulder", "X", [0] * 20)
    with pytest.raises(ValueError, match="at most 20 values"):
        Command("Shoulder", "X", [0] * 21)


def test_bad_value_rejected_at_construction():
    with pytest.raises(TypeError):
        Command("Shoulder", "MoveOne", ["fast"])  # ty: ignore


# --- LoadTicket packet ---


def expected_packet(
    ticket: int, joint: bytes, name: bytes, values: bytes, n_values: int
) -> bytes:
    return (
        b"LoadTicket"
        + ticket.to_bytes(4, "little")
        + joint.ljust(50, b"\0")
        + bytes([len(name), n_values])
        + name
        + values
    )


def test_pack_move_one():
    command = Command("Shoulder", "MoveOne", [-23.0, 40, 3])
    values = b"F" + f32(-23.0) + b"I" + i32(40) + b"I" + i32(3)
    assert command.pack(7) == expected_packet(7, b"Shoulder", b"MoveOne", values, 3)


def test_pack_no_values():
    command = Command("Hip", "GetAngle")
    assert command.pack(0) == expected_packet(0, b"Hip", b"GetAngle", b"", 0)


def test_pack_max_ticket():
    command = Command("Hip", "GetAngle")
    assert command.pack(2**32 - 1)[10:14] == b"\xff\xff\xff\xff"


@pytest.mark.parametrize("ticket", [-1, 2**32])
def test_pack_bad_ticket(ticket):
    with pytest.raises(ValueError, match="ticket"):
        Command("Hip", "GetAngle").pack(ticket)


def test_pack_utf8_joint():
    command = Command("Épaule", "GetAngle")
    assert command.pack(1) == expected_packet(1, "Épaule".encode(), b"GetAngle", b"", 0)


def test_pack_largest_command_fits_in_a_frame():
    command = Command("j" * 49, "n" * 254, [0] * 20)
    assert len(command.pack(0)) <= 1024


def test_pack_matches_legacy_item_format():
    """Same bytes the original Crab.send LoadTicket loop produced for int/float values."""
    ticket, joint, name, values = 7, "Shoulder", "MoveOne", [-23.0, 40, 3]
    legacy = b"LoadTicket"
    legacy += struct.pack("I", ticket)
    legacy += struct.pack("50s", joint.encode())
    legacy += struct.pack("B", len(name))
    legacy += struct.pack("B", len(values))
    legacy += name.encode()
    legacy += b"F" + struct.pack("f", values[0])
    legacy += b"I" + struct.pack("i", values[1])
    legacy += b"I" + struct.pack("i", values[2])
    assert Command(joint, name, values).pack(ticket) == legacy
