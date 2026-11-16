"""Command builders for HerkuleX smart servos.

Each builder returns a ``Command`` whose values are laid out the way
``Urchin_Firmware/src/Conversation/UnPacker.cpp::LoadTicket`` reads them. The
firmware resolves the joint to a motor number, model, and bounds via the bridge.
"""

from __future__ import annotations

from enum import Enum, IntEnum

from crab_api2.command import Command

MAX_PLAY_TIME = 255  # ticks; the firmware multiplies by PTime (11.2 ms), max 2856 ms


class HerkulexCommand(str, Enum):
    MOVE_ONE = "MoveOne"
    GET_ANGLE = "GetAngle"
    SET_TORQUE = "SetTorque"
    TEST_REACT = "TestReact"

    # Stringify to the firmware name (like 3.11's StrEnum), not "HerkulexCommand.MOVE_ONE"
    __str__ = str.__str__
    __format__ = str.__format__


class LedColor(IntEnum):
    """``LedColor`` in Herkulex.h."""

    LED_GREEN_HRAMWRITE = 1
    LED_BLUE_HRAMWRITE = 2
    LED_CYAN_HRAMWRITE = 3
    LED_RED_HRAMWRITE = 4
    LED_GREEN2_HRAMWRITE = 5
    LED_PINK_HRAMWRITE = 6
    LED_WHITE_HRAMWRITE = 7


class JogLedColor(IntEnum):
    """``JogLedColor`` in Herkulex.h: LED color while moving."""

    LED_GREEN = 2
    LED_BLUE = 3
    LED_RED = 4


class HerkulexModel(IntEnum):
    """``HerkulexModel`` in Herkulex.h."""

    MODEL_0101 = 0
    MODEL_0201 = 1
    MODEL_0601 = 2
    MODEL_0602 = 3


class _Torque(IntEnum):
    """``SetTorque`` codes handled by LoadTicket."""

    OFF = 0
    ON = 1
    ALL_OFF = 2
    ALL_ON = 3


def move_one(
    joint: str, goal: float, play_time: int, color: JogLedColor | int
) -> Command:
    """Move one joint to ``goal`` degrees.

    ``play_time`` is in HerkuleX ticks of 11.2 ms (0-255). The firmware rejects goals
    outside the joint's bridge ``Bounds``.
    """
    if isinstance(goal, bool) or not isinstance(goal, int | float):
        raise TypeError(f"goal must be a number, got {type(goal).__name__}")
    if isinstance(play_time, bool) or not isinstance(play_time, int):
        raise TypeError(f"play_time must be an int, got {type(play_time).__name__}")
    if not 0 <= play_time <= MAX_PLAY_TIME:
        raise ValueError(
            f"play_time must be between 0 and {MAX_PLAY_TIME} ticks (11.2 ms each), "
            f"got {play_time}"
        )
    try:
        color = JogLedColor(color)
    except ValueError:
        allowed = ", ".join(str(c.value) for c in JogLedColor)
        raise ValueError(
            f"color must be a JogLedColor ({allowed}), got {color}"
        ) from None
    # The firmware reads the goal as a float, so an int must not go out with an int tag
    return Command(
        joint, HerkulexCommand.MOVE_ONE, (float(goal), play_time, int(color))
    )


def get_angle(joint: str) -> Command:
    """Read a joint's angle. The firmware currently only logs it to the debug stream."""
    return Command(joint, HerkulexCommand.GET_ANGLE)


def set_torque(joint: str, on: bool, broadcast: bool = False) -> Command:
    """Turn torque on or off for ``joint``, or for every motor when ``broadcast`` is set.

    A broadcast still needs a joint from the bridge: the firmware looks it up before
    acting on the broadcast.
    """
    if broadcast:
        code = _Torque.ALL_ON if on else _Torque.ALL_OFF
    else:
        code = _Torque.ON if on else _Torque.OFF
    return Command(joint, HerkulexCommand.SET_TORQUE, (int(code),))


def test_react(joint: str, value: int) -> Command:
    """Ask the firmware to send ``value`` back as a reaction (code 45). For testing reactions."""
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"value must be an int, got {type(value).__name__}")
    return Command(joint, HerkulexCommand.TEST_REACT, (value,))
