"""Byte-exact checks of the BridgeAdd packet against the firmware's BridgeMotor layout.

Urchin_Firmware/src/Global/Bridge.h (packed, little-endian):
    unsigned char Num;      1
    char Brand[30];        30
    char Model[10];        10
    char Joint[50];        50
    int BoundsMin;          4
    int BoundsMax;          4
    int AlignmentAngle;     4
    bool NextNode;          1   -> 104 bytes

UnPacker.cpp reads: "Bridge" command, then "Add", then a uint32 payload length, then the struct.
"""

import pytest

from crab_api2.bridge import Motor

FIRMWARE_STRUCT_SIZE = 104


def int32(value: int) -> bytes:
    return value.to_bytes(4, "little", signed=True)


def expected_packet(motor: Motor, last: bool) -> bytes:
    payload = (
        bytes([motor.number])
        + motor.brand.encode().ljust(30, b"\0")
        + motor.model.encode().ljust(10, b"\0")
        + motor.joint.encode().ljust(50, b"\0")
        + int32(motor.bounds[0])
        + int32(motor.bounds[1])
        + int32(motor.alignment_angle)
        + (b"\x01" if last else b"\x00")
    )
    return b"BridgeAdd" + len(payload).to_bytes(4, "little") + payload


SHOULDER = Motor(7, "HerkuleX", "0601", "Shoulder", (-45, 45), 0)


@pytest.mark.parametrize("last", [False, True])
def test_pack_matches_firmware_layout(last):
    assert SHOULDER.pack(last) == expected_packet(SHOULDER, last)


def test_payload_size_matches_firmware_struct():
    packet = SHOULDER.pack(False)
    assert packet.startswith(b"BridgeAdd")
    length = int.from_bytes(packet[9:13], "little")
    assert length == FIRMWARE_STRUCT_SIZE
    assert len(packet) == 9 + 4 + FIRMWARE_STRUCT_SIZE


def test_pack_extremes():
    motor = Motor(253, "b" * 29, "m" * 9, "j" * 49, (-(2**31), 2**31 - 1), -90)
    assert motor.pack(True) == expected_packet(motor, True)


def test_pack_utf8_joint():
    motor = Motor(1, "HerkuleX", "0601", "Épaule", (-10, 10), 5)
    assert motor.pack(False) == expected_packet(motor, False)


def test_pack_matches_legacy_bridge_format():
    """Same bytes the original Crab_API Bridge.Send produced."""
    import struct

    legacy_payload = struct.pack(
        "<B30s10s50siii?", 7, b"HerkuleX", b"0601", b"Shoulder", -45, 45, 0, True
    )
    legacy = b"BridgeAdd" + struct.pack("<I", len(legacy_payload)) + legacy_payload
    assert SHOULDER.pack(True) == legacy
