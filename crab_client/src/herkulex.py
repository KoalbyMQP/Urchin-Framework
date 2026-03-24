from __future__ import annotations

from enum import IntEnum

from .types import CommandItem


class LedColor(IntEnum):
    LED_GREEN_HRAMWRITE = 1
    LED_BLUE_HRAMWRITE = 2
    LED_CYAN_HRAMWRITE = 3
    LED_RED_HRAMWRITE = 4
    LED_GREEN2_HRAMWRITE = 5
    LED_PINK_HRAMWRITE = 6
    LED_WHITE_HRAMWRITE = 7


class JogLedColor(IntEnum):
    LED_GREEN = 2
    LED_BLUE = 3
    LED_RED = 4


class HerkulexModel(IntEnum):
    MODEL_0101 = 0
    MODEL_0201 = 1
    MODEL_0601 = 2
    MODEL_0602 = 3


class HerkuleX:
    @staticmethod
    def move_one(joint_name: str, goal: float, time_ms: int, color: JogLedColor) -> CommandItem:
        if (time_ms * 11.2 > 500) or (time_ms * 11.2 < 0):
            raise ValueError("pTime must be between 500 and 0.")
        return CommandItem.from_primitives(joint_name, "MoveOne", [goal, time_ms, int(color)])
