import Reaction


class React:
    def __init__(self,ticket,resolver):
        if not isinstance(ticket,int):
            raise TypeError('ticket must be an integer')

        if not isinstance(resolver,list):
            raise TypeError("resolver must be a list")
        if not all(isinstance(hold,Reaction) for hold in resolver):
            raise TypeError("all elements in resolver must be a\"Reaction\"")

        self.ticket=ticket
        self.resolver=resolver