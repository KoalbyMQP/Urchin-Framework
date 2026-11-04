"""HerkuleX builders, checked against how UnPacker.cpp::LoadTicket reads each command's values."""

import struct

import pytest

from crab_api2 import herkulex
from crab_api2.command import Command
from crab_api2.herkulex import HerkulexCommand, HerkulexModel, JogLedColor, LedColor


def encoded_values(command: Command) -> bytes:
    """The value section of the LoadTicket packet (after the 66-byte prefix + name)."""
    return command.pack(0)[10 + 4 + 50 + 2 + len(command.name) :]


# --- enums mirror Herkulex.h ---


def test_jog_led_color_values():
    assert {c.name: c.value for c in JogLedColor} == {
        "LED_GREEN": 2,
        "LED_BLUE": 3,
        "LED_RED": 4,
    }


def test_led_color_values():
    assert [c.value for c in LedColor] == [1, 2, 3, 4, 5, 6, 7]


def test_model_values():
    assert {m.name: m.value for m in HerkulexModel} == {
        "MODEL_0101": 0,
        "MODEL_0201": 1,
        "MODEL_0601": 2,
        "MODEL_0602": 3,
    }


def test_command_names_match_firmware_strings():
    assert set(HerkulexCommand) == {"MoveOne", "GetAngle", "SetTorque", "TestReact"}


# --- move_one: Variables[0].Float goal, [1].Int play time, [2].Int JogLedColor ---


def test_move_one():
    command = herkulex.move_one("Shoulder", -23.0, 40, JogLedColor.LED_BLUE)
    assert command == Command("Shoulder", "MoveOne", (-23.0, 40, 3))
    assert encoded_values(command) == (
        b"F"
        + struct.pack("<f", -23.0)
        + b"I"
        + struct.pack("<i", 40)
        + b"I"
        + struct.pack("<i", 3)
    )


def test_move_one_int_goal_sent_as_float():
    # The firmware reads the goal as a float; an int tag would be misread as float bits
    command = herkulex.move_one("Shoulder", 23, 40, JogLedColor.LED_RED)
    assert encoded_values(command)[:5] == b"F" + struct.pack("<f", 23.0)


@pytest.mark.parametrize("play_time", [0, 255])
def test_move_one_play_time_limits(play_time):
    assert (
        herkulex.move_one("Hip", 0.0, play_time, JogLedColor.LED_GREEN).values[1]
        == play_time
    )


@pytest.mark.parametrize("play_time", [-1, 256])
def test_move_one_play_time_out_of_range(play_time):
    with pytest.raises(ValueError, match="play_time must be between 0 and 255 ticks"):
        herkulex.move_one("Hip", 0.0, play_time, JogLedColor.LED_GREEN)


@pytest.mark.parametrize("play_time", [40.0, True])
def test_move_one_play_time_must_be_int(play_time):
    with pytest.raises(TypeError, match="play_time must be an int"):
        herkulex.move_one("Hip", 0.0, play_time, JogLedColor.LED_GREEN)


def test_move_one_goal_must_be_number():
    with pytest.raises(TypeError, match="goal must be a number"):
        herkulex.move_one("Hip", "23", 40, JogLedColor.LED_GREEN)


def test_move_one_accepts_color_int():
    assert herkulex.move_one("Hip", 0.0, 40, 4).values[2] == 4


def test_move_one_rejects_unknown_color():
    with pytest.raises(ValueError, match="color"):
        herkulex.move_one("Hip", 0.0, 40, 7)


# --- get_angle: no values (fixes the old builder, which crashed) ---


def test_get_angle():
    assert herkulex.get_angle("Hip") == Command("Hip", "GetAngle", ())


# --- set_torque: Variables[0].Int 0 off, 1 on, 2 all off, 3 all on ---


@pytest.mark.parametrize(
    ("on", "broadcast", "code"),
    [(False, False, 0), (True, False, 1), (False, True, 2), (True, True, 3)],
)
def test_set_torque_codes(on, broadcast, code):
    command = herkulex.set_torque("Shoulder", on, broadcast=broadcast)
    assert command == Command("Shoulder", "SetTorque", (code,))
    assert encoded_values(command) == b"I" + struct.pack("<i", code)


def test_set_torque_broadcast_requires_joint():
    with pytest.raises(ValueError, match="joint must not be empty"):
        herkulex.set_torque("", True, broadcast=True)


# --- test_react: Variables[0].Int echoed back in a reaction ---


def test_test_react():
    command = herkulex.test_react("Shoulder", 69)
    assert command == Command("Shoulder", "TestReact", (69,))
    assert encoded_values(command) == b"I" + struct.pack("<i", 69)


def test_test_react_value_must_be_int():
    with pytest.raises(TypeError, match="value must be an int"):
        herkulex.test_react("Shoulder", 6.9)
