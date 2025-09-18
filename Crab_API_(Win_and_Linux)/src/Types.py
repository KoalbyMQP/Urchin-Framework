from typing import List, Union, Callable


class Item:
    """

    """
    def __init__(self,motor: int, command: str, values: List[int | float | str ]) -> None:
        """
        Makes a smart item
        :param motor: motor ID number
        :param command: Command name
        :param values: list of values for command
        """
        self.motor = motor
        self.command = command
        self.values = values

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



