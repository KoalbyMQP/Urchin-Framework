from enum import IntEnum, StrEnum
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

class HerkuleXCommandNames(StrEnum):
    MoveOne: str = "MoveOne"
    GetAngle: str = "GetAngle"
    SetTorque: str = "SetTorque"
    TestReact: str = "TestReact"

class HerkuleX():

    def MoveOne(JointName: str, Goal: float, Time_ms: int, color: JogLedColor) -> Item:
        """
        Prepares a command that moves one motor
        :param JointName: The joint name
        :param Goal: goal position of the motor in degrees
        :param Time_ms: Time in milliseconds that the motor is moving
        :param color: Color of the motor LED
        :return: The Item to send to Urchin framework
        """
        Name: str = HerkuleXCommandNames.MoveOne

        if ((Time_ms*11.2 > 500) or (Time_ms*11.2 < 0)):
            raise ValueError("pTime must be between 500 and 0.")

        command: Item = Item(JointName,Name,[Goal, Time_ms, color])
        return command

    def GetAngle(JointName: str)-> Item:
        """
        Prepares a command that gets the angle of the motor
        :return: The Item to send to Urchin framework
        """
        Name: str = HerkuleXCommandNames.GetAngle
        return Item(JointName,Name)

    def SetTorque(value: bool, JointName: str = "", BrodcastFlag: bool = False) -> Item:
        """
        Sets the torque to a value
        :param value: Torque setting
        :param JointName: The joint name
        :param BrodcastFlag: If SetTorque is targeting one motor or all motors
        :return:
        """
        # 0 = JointName OFF, 1 = JointName ON, 2 = All OFF, 3 = All ON

        if not BrodcastFlag:
            output = 1 if value else 0
        else:
            output = 3 if value else 2

        Name: str = HerkuleXCommandNames.SetTorque
        return Item(JointName, Name, [output])

    def TestReact(JointName: str, value: int) -> Item:
        """
        Sets the torque to a value
        :param JointName: The joint name
        :return:
        """
        Name: str = HerkuleXCommandNames.TestReact
        return Item(JointName, Name, [value])