import queue
import threading


import serial
from numpy import character
from pandas.conftest import tick_classes

from ESPSerial import *
from Types import *
from enum import Enum
from enum import StrEnum

class TicketType(StrEnum):
    Sequential = "S"
    Resolving = "R"
    Asynchronous = "A"
    Interrupt = "I"



class Crab:
    


    def __init__(self, dev: bool = False) -> None:

        self.serial = ESPSerial(dev)
        self.react: list[React] = []
        #print("before Starting Smart Thread")


        self.Alive = True
        self.dev = dev

        self.Exchange = queue.Queue()
        self.Reaction = queue.Queue()
        self.Debug = queue.Queue()

        self.Thread: threading.Thread = threading.Thread(target=self._SmartRuner)
        self.Thread.start()


    def _QueSmartPop(self, queue: queue.Queue[Any],Format: str) -> Any:
        while queue.empty():
            pass
        buffer: str =  queue.get()

        if (Format == b'S'):
            return bytes(buffer).decode('utf-8').rstrip('\x00')

        buffer: bytes = bytes(buffer[:(struct.calcsize(Format))])
        return struct.unpack(Format, buffer)[0]


    def _SmartRuner(self) -> None:
        '''

        :return: void
        '''





        while(self.Alive):

            packet=self.serial.read_packet()

            if packet is not None:

                if (packet["Stream"]==b'E'):

                    self.Exchange.put(packet["data_str"])

                if (packet["Stream"] == b'R'):

                    self.Reaction.put(packet["data_str"])

                if (packet["Stream"] == b'D'):

                    self.Debug.put(packet["data_str"])

        self.serial.close()






    def _CheckReaction(self,code: int):
        for element in self.react:
            print(element)


    def send(self,type: TicketType ,items: List[Item] ,resolver : React | None, chained: bool )-> int:
        '''
        Used to send a motor control command to the Esp32
        :param type: the Type of command "I" for "Interrupt","S" for "Sequential","R" for "Resolving","A" for "Asynchronous"
        :param items: list of items, see item class
        :param resolver: list of reactions used when a ticket is closed
        :param chained: if true function will hold till ticket is closed, if false function will not hold.
        :return: ticket number
        '''

        Ticket: int =-1





        # todo
        # add restraints later



        # todo
        #request ticket
        self.serial.send_packet(0,b"ReqTicket")

        #Check Error
        Error: int = self. _QueSmartPop(self.Exchange,"<i")
        if (Error == 1):
            return -1

        #Receve Ticket
        ticket = self._QueSmartPop(self.Exchange,"<I")

        if self.dev:
            print("Ticket:" + str(ticket))


        #Format ticket
        if self.dev:
            print("sending:"+ "FormatTicket" + struct.pack('i', ticket).decode('latin-1') + type)
        self.serial.send_packet(0, b"FormatTicket" + struct.pack('i', ticket) + type.encode('utf-8'))
        # Load ticket


        for item in items:
            Strip: str = b"LoadTicket"
            Strip += struct.pack('I', ticket)  # Unsigned int
            Strip += struct.pack('I', item.motor) # Unsigned int
            Strip += struct.pack('I', item.model) # Unsigned int
            Strip += struct.pack('B', len(item.command)) #Int of 1 byte
            Strip += struct.pack('B', len(item.values))  # Int of 1 byte
            Strip += item.command.encode('utf-8') #string
            for point in item.values:
                Strip += item.Press(point)
            self.serial.send_packet(0,  Strip)







        # punch ticket
        self.serial.send_packet(0, b"PunchTicket"+struct.pack('I', ticket))


        #todo
        #add resolver to the self.react



        #todo
        return 0


    def CloseTicket(self,ticket):
        if not isinstance(ticket,int):
            raise TypeError("ticket must be an int")

        #todo
        #send "CloseTicket" command to esp32

    def TicketInfo(self,ticket):
        if not isinstance(ticket,int):
            raise TypeError("ticket must be an int")

        #todo
        #request ticket status

    def GetHealth(self,type):
        if not isinstance(type,str):
            raise TypeError("type must be str")

        if type not in ["TotalRam","FreeRam","CPU","AckCheck"]:
            raise TypeError("Type must be one of TotalRam,FreeRam,CPU,AckCheck")

        #todo
        #send "GetHealth" to esp32 along with the type
        self.serial.write(("GetHealth"+type).encode('utf-8'))

        return self.serial.readline()

    def close(self):
         self.Alive=False
         self.Thread.join()