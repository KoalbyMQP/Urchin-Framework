from typing import List, Union, Callable


class Item:
    def __init__(self,motor: int, command: str ,values: List[Union[int,float]]) -> None:
        self.motor = motor
        self.command = command
        self.values = values




class Reaction:
    def __init__(self, codes :List[int], function: Callable[[str],int]) -> None:


        #deprecated due to Typing
        # if not callable(function):
        #     raise TypeError("function must be a function")
        #
        # if not isinstance(codes, list):
        #     raise TypeError("codes must be a list")
        # if not all(isinstance(code,int) for code in codes):
        #     raise TypeError("all elements in codes must be an int")



        self.codes = codes
        self.function = function





class React:
    def __init__(self,ticket: int,resolver: List[Reaction]) -> None:

        # deprecated due to Typing
        # if not isinstance(ticket,int):
        #     raise TypeError('ticket must be an integer')
        #
        # if not isinstance(resolver,list):
        #     raise TypeError("resolver must be a list")
        # if not all(isinstance(hold,Reaction) for hold in resolver):
        #     raise TypeError("all elements in resolver must be a\"Reaction\"")

        self.ticket=ticket
        self.resolver=resolver