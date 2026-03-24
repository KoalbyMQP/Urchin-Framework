from enum import IntEnum
from Types import Item

class LedColor(IntEnum):
    LED_GREEN_HRAMWRITE: int = 1  # Green
    LED_BLUE_HRAMWRITE: int = 2  # Blue
    LED_CYAN_HRAMWRITE: int = 3  # Cyan
    LED_RED_HRAMWRITE: int = 4  # Red
    LED_GREEN2_HRAMWRITE: int = 5  # Soft Green
    LED_PINK_HRAMWRITE: int = 6  # Pink
    LED_WHITE_HRAMWRITE: int = 7  # White

class JogLedColor(IntEnum):
    LED_GREEN: int = 2,
    LED_BLUE: int = 3,
    LED_RED: int = 4

class HerkulexModel(IntEnum):
    MODEL_0101: int = 0
    MODEL_0201: int = 1
    MODEL_0601: int = 2
    MODEL_0602: int = 3

class HerkuleX():

    def MoveOne(JoinyName: str, Goal: float, Time_ms: int, color: JogLedColor) -> Item:
        Name: str = "MoveOne"

        if ((Time_ms*11.2 > 500) or (Time_ms*11.2 < 0)):
            raise ValueError("pTime must be between 500 and 0.")

        command: Item = Item(JoinyName,Name,[Goal, Time_ms, color])
        return command
