class Reaction:
    def __init__(self, codes, function):


        if not callable(function):
            raise TypeError("function must be a function")

        if not isinstance(codes, list):
            raise TypeError("codes must be a list")
        if not all(isinstance(code,int) for code in codes):
            raise TypeError("all elements in codes must be an int")

        self.codes = codes
        self.function = function
