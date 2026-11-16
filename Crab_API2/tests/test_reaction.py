"""Reaction decoding against Urchin_Firmware/src/ESP_PI_Communication/Reaction.cpp::SendReaction.

PacketHeader (packed, little-endian):
    unsigned int ticket;        4
    unsigned int Code;          4
    char joint[50];            50
    unsigned char command_len;  1
    unsigned char values_len;   1   -> 60 bytes
char command[command_len]
Data[values_len]: char Type ('i'/'f'/'b'); union {int; float; char}   -> 5 bytes each
"""

import logging
import struct

import pytest

from crab_api2.herkulex import HerkulexCommand
from crab_api2.reaction import (
    Reaction,
    ReactionError,
    ReactionEvent,
    ReactionRegistry,
    decode_reaction,
)


def reaction_packet(
    ticket: int, code: int, joint: bytes, command: bytes, values: bytes, n_values: int
) -> bytes:
    """A reaction payload as SendReaction lays it out, zero-padded like a 1024-byte frame."""
    packet = (
        ticket.to_bytes(4, "little")
        + code.to_bytes(4, "little")
        + joint.ljust(50, b"\0")
        + bytes([len(command), n_values])
        + command
        + values
    )
    return packet.ljust(1024, b"\0")


def i_value(v: int) -> bytes:
    return b"i" + v.to_bytes(4, "little", signed=True)


def f_value(v: float) -> bytes:
    return b"f" + struct.pack("<f", v)


def b_value(v: int, junk: bytes = b"\0\0\0") -> bytes:
    # SendReaction only sets the first union byte; the rest is uninitialized memory
    return b"b" + v.to_bytes(1, "little", signed=True) + junk


# --- decoding ---


def test_decode_test_react():
    # SendReaction(VPID, ticket, "Shoulder", "TestReact", 9, 45, "%i", 69)
    data = reaction_packet(7, 45, b"Shoulder", b"TestReact", i_value(69), 1)
    assert decode_reaction(data) == ReactionEvent(
        ticket=7, code=45, joint="Shoulder", command="TestReact", values=(69,)
    )


def test_decode_mixed_values_stride_5_bytes():
    values = (
        i_value(-5)
        + f_value(1.5)
        + b_value(-1, junk=b"\xde\xad\xbe")
        + i_value(2**31 - 1)
    )
    data = reaction_packet(1, 2, b"Hip", b"GetAngle", values, 4)
    assert decode_reaction(data).values == (-5, 1.5, -1, 2**31 - 1)


def test_decode_byte_value_is_signed():
    data = reaction_packet(0, 0, b"Hip", b"X", b_value(127) + b_value(-128), 2)
    assert decode_reaction(data).values == (127, -128)


def test_decode_no_values():
    data = reaction_packet(0, 0, b"Hip", b"X", b"", 0)
    assert decode_reaction(data).values == ()


def test_decode_max_ticket_and_code():
    data = reaction_packet(2**32 - 1, 2**32 - 1, b"Hip", b"X", b"", 0)
    event = decode_reaction(data)
    assert (event.ticket, event.code) == (2**32 - 1, 2**32 - 1)


def test_decode_full_width_joint():
    # strncpy(joint, Joint, 49) leaves at least one NUL; 49 chars is the maximum
    data = reaction_packet(0, 0, b"j" * 49, b"X", b"", 0)
    assert decode_reaction(data).joint == "j" * 49


def test_decode_unpadded_payload():
    # Exactly header + command + one value, with no frame padding
    packet = reaction_packet(3, 45, b"Hip", b"TestReact", i_value(1), 1)[: 60 + 9 + 5]
    assert decode_reaction(packet).values == (1,)


def test_decode_unknown_tag():
    data = reaction_packet(0, 0, b"Hip", b"X", b"q\0\0\0\0", 1)
    with pytest.raises(ReactionError, match="unknown value type 'q'"):
        decode_reaction(data)


def test_decode_too_short_for_header():
    with pytest.raises(ReactionError, match="too short"):
        decode_reaction(b"\0" * 59)


def test_decode_values_past_end():
    packet = reaction_packet(0, 0, b"Hip", b"X", i_value(1), 1)[: 60 + 1 + 5]
    with pytest.raises(ReactionError, match="too short"):
        decode_reaction(packet[:-1])


# --- Reaction matching ---


def event(joint="Shoulder", command="TestReact", code=45, ticket=7, values=(69,)):
    return ReactionEvent(
        ticket=ticket, code=code, joint=joint, command=command, values=values
    )


def noop(event: ReactionEvent) -> None:
    pass


def test_reaction_normalizes_codes():
    reaction = Reaction("Shoulder", "TestReact", [45, 46, 45], noop)
    assert reaction.codes == frozenset({45, 46})


def test_reaction_accepts_command_enum():
    reaction = Reaction("Shoulder", HerkulexCommand.TEST_REACT, [45], noop)
    assert reaction.matches(event())


@pytest.mark.parametrize(
    "other",
    [event(joint="Hip"), event(command="GetAngle"), event(code=46)],
)
def test_reaction_mismatch(other):
    assert not Reaction("Shoulder", "TestReact", [45], noop).matches(other)


def test_reaction_empty_codes_match_any():
    reaction = Reaction("Shoulder", "TestReact", [], noop)
    assert reaction.matches(event(code=1)) and reaction.matches(event(code=999))


def test_reaction_ignores_ticket():
    assert Reaction("Shoulder", "TestReact", [45], noop).matches(event(ticket=12345))


def test_reaction_is_immutable():
    reaction = Reaction("Shoulder", "TestReact", [45], noop)
    with pytest.raises(AttributeError):
        reaction.joint = "Hip"  # type: ignore[misc]


# --- registry ---


def recorder():
    calls: list[ReactionEvent] = []
    return calls, calls.append


def test_dispatch_to_matching_ticket_only():
    calls, callback = recorder()
    registry = ReactionRegistry()
    registry.add(7, [Reaction("Shoulder", "TestReact", [45], callback)])
    assert registry.dispatch(event(ticket=8)) == 0
    assert registry.dispatch(event(ticket=7)) == 1
    assert calls == [event(ticket=7)]


def test_dispatch_runs_every_match():
    calls, callback = recorder()
    registry = ReactionRegistry()
    registry.add(
        7,
        [
            Reaction("Shoulder", "TestReact", [45], callback),
            Reaction("Shoulder", "TestReact", [], callback),
            Reaction("Hip", "TestReact", [45], callback),
        ],
    )
    assert registry.dispatch(event()) == 2
    assert len(calls) == 2


def test_reaction_stays_until_ticket_removed():
    calls, callback = recorder()
    registry = ReactionRegistry()
    registry.add(7, [Reaction("Shoulder", "TestReact", [45], callback)])
    registry.dispatch(event())
    registry.dispatch(event())
    registry.remove_ticket(7)
    assert registry.dispatch(event()) == 0
    assert len(calls) == 2


def test_remove_unknown_ticket_is_noop():
    ReactionRegistry().remove_ticket(99)


def test_add_twice_extends_ticket():
    _calls, callback = recorder()
    registry = ReactionRegistry()
    registry.add(7, [Reaction("Shoulder", "TestReact", [45], callback)])
    registry.add(7, [Reaction("Shoulder", "TestReact", [], callback)])
    assert registry.dispatch(event()) == 2


def test_same_reaction_reused_across_tickets():
    _calls, callback = recorder()
    reaction = Reaction("Shoulder", "TestReact", [45], callback)
    registry = ReactionRegistry()
    registry.add(1, [reaction])
    registry.add(2, [reaction])
    registry.remove_ticket(1)
    assert registry.dispatch(event(ticket=1)) == 0
    assert registry.dispatch(event(ticket=2)) == 1


def test_callback_exception_is_logged_and_others_run(caplog):
    calls, callback = recorder()

    def broken(event: ReactionEvent) -> None:
        raise RuntimeError("boom")

    registry = ReactionRegistry()
    registry.add(
        7,
        [
            Reaction("Shoulder", "TestReact", [45], broken),
            Reaction("Shoulder", "TestReact", [45], callback),
        ],
    )
    with caplog.at_level(logging.ERROR, logger="crab_api2.reaction"):
        assert registry.dispatch(event()) == 2
    assert len(calls) == 1
    assert "boom" in caplog.text


def test_callback_may_modify_registry():
    registry = ReactionRegistry()

    def close_own_ticket(event: ReactionEvent) -> None:
        registry.remove_ticket(event.ticket)
        registry.add(event.ticket + 1, [])

    registry.add(7, [Reaction("Shoulder", "TestReact", [45], close_own_ticket)])
    assert registry.dispatch(event()) == 1
    assert registry.dispatch(event()) == 0
