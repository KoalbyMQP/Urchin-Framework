
from enum import Enum
from enum import IntEnum
from Types import Item

class LedColor(IntEnum):
    LED_GREEN_HRAMWRITE  = 1  # Green
    LED_BLUE_HRAMWRITE   = 2  # Blue
    LED_CYAN_HRAMWRITE   = 3  # Cyan
    LED_RED_HRAMWRITE    = 4  # Red
    LED_GREEN2_HRAMWRITE = 5  # Soft Green
    LED_PINK_HRAMWRITE   = 6  # Pink
    LED_WHITE_HRAMWRITE  = 7  # White

class JogLedColor(IntEnum):
    LED_GREEN = 2,
    LED_BLUE = 3,
    LED_RED = 4

class HerkulexModel(IntEnum):
  MODEL_0101 = 0
  MODEL_0201 = 1
  MODEL_0601 = 2
  MODEL_0602 = 3

class HerkuleX():

    def MoveOne(servoID: int, Goal: int, pTime: int, color: JogLedColor, Model: HerkulexModel)-> Item:

        Name: str = "MoveOne"
        MaxServoID: int =245
        if servoID < 1:
            raise ValueError("servoID must be a positive integer.")
        if servoID > MaxServoID:
            raise ValueError("servoID less then",str(MaxServoID))

        if ((pTime > 50) or (pTime < 0)):
            raise ValueError("pTime must be between 50 and 0.")

        command: Item = Item(servoID,Model,Name,[Goal, pTime, color])
        return command

