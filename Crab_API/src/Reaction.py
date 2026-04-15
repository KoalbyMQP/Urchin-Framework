import inspect
from typing import Callable, Any, List, Tuple, get_origin


class Reaction:

    def __init__(self, Joint: str, commandName: str, CommandCodes: list[int], function: Callable[[Any], Any]) -> None:
        """

        :param Joint:
        :param commandName:
        :param CommandCodes:
        :param function:
        """

        self.Joint = Joint
        self.CommandCodes = CommandCodes
        self.CommandName = commandName
        self.Function = function



    def SetTicket(self,TicketNum: int) -> None:
        self.TicketNum = TicketNum

    def CompareIncomeing(self, TicketNum: int, Joint: str, CommandName: str, CommandCode: int, expected_params: List[Tuple[type, str]]) -> bool:
        if (self.TicketNum != TicketNum):
            return False
        if (self.Joint != Joint):
            return False
        if (self.CommandName != CommandName):
            return False
        if (not CommandCode in self.CommandCodes):
            return False

        if (not compare_func_params(expected_params)):
            return False

        return True




    def compare_func_params(self, expected_params: List[Tuple[type, str]]) -> bool:
        """
        Return True if function parameters match expected (type, name) pairs.

        expected_params format:
            [(type, "param_name"), ...]
        """

        sig = inspect.signature(self.Function)
        actual_params = sig.parameters

        # Quick length check (fast fail)
        if len(actual_params) != len(expected_params):
            return False

        def type_matches(actual_type, expected_type):
            # No annotation → allow (or change to False if you want strict)
            if actual_type is inspect._empty:
                return True

            # Handle Callable
            if expected_type is Callable:
                return (
                        actual_type is self.Function or
                        get_origin(actual_type) is self.Function
                )

            return actual_type == expected_type

        # Compare in order
        for (expected_type, expected_name), (actual_name, param) in zip(
                expected_params, actual_params.items()
        ):
            # Name mismatch
            if expected_name != actual_name:
                return False

            # Type mismatch
            if not type_matches(param.annotation, expected_type):
                return False

        return True

