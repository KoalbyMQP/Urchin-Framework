class Reaction:

    def __init__(self, TicketNum: int, commandName: str, function: Callable[[any], any]) -> None:
        """
        Makes a smart item
        :param joint: motor joint name
        :param command: Command name
        :param values: list of values for command
        """
        self.TicketNum = TicketNum
        self.commandName = commandName
        self.function = function


