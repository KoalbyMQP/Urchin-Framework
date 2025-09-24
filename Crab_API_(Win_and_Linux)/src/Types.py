from typing import List, Union, Callable, Optional
import struct
from enum import IntEnum

class Item:
    """

    """
    def __init__(self, motor: int, model: int, command: str, values: List[Union[int, float, bool]]) -> None:
        """
        Makes a smart item
        :param motor: motor ID number
        :param model: model ID number
        :param command: Command name
        :param values: list of values for command
        """
        self.motor = motor
        self.model = model
        self.command = command
        self.values = values
        for i in range(len(self.values)):
            if isinstance(self.values[i], IntEnum):
                self.values[i] = int(self.values[i])


    def Press(self, Instance: Union[int, float, bool]) -> Optional[bytes]:
            # Removed due to complexity
        # if(type(Instance) == str):
        #     return "S"+str(len(Instance))+Instance
        if (type(Instance) == int):
            bytes = struct.pack('i', Instance)
            return b"I"+bytes
        if (type(Instance) == float):
            bytes = struct.pack('f', Instance)
            return b"F"+bytes
        if (type(Instance) == bool):
            bytes = struct.pack('B', Instance)
            return b"B" + bytes


class Reaction:
    def __init__(self, codes: List[int], function: Callable[[str],int]) -> None:
        self.codes = codes
        self.function = function


class React:
    def __init__(self, ticket: int, resolver: List[Reaction]) -> None:
        self.ticket = ticket
        self.resolver = resolver


class Restraints:
    type: str
    value: int
    def __init__(self,type:str, value: int) -> None:
        self.type = type
        self.value = value



