class Memory():
    def __init__(self, Type, Bytes, ROM, RAM, Min, Max, Perm, Fucntion):
        """
        constructor for Memory class
        :param Type: Base name
        :param Bytes: Size of value
        :param ROM: ROM Address
        :param RAM: RAM Address
        :param Max: Max Value
        :param Min: Min Value
        :param Perm: Perm Value "RO" | "RW" | "NONE"
        :param Fucntion: Fucntion pointer
        """
        self.Type = Type
        self.Bytes = Bytes
        self.ROM = ROM
        self.RAM = RAM
        self.Max = Max
        self.Min = Min
        self.Perm = Perm
        self.Fucntion = Fucntion

    def __repr__(self):
        "Memory:Type %s, Bytes %d, ROM %d, RAM %d, Max %d, Perm %s" % (self.Type, self.Bytes, self.ROM, self.RAM, self.Max, self.Perm)
