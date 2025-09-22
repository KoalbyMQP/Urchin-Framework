import queue
import threading

import serial
from pandas.conftest import tick_classes

from ESPSerial import *
from Types import *


class Smart:
    
    def __init__(self)->None:
        #todo
        #start the listener loop
        self.serial = ESPSerial()
        self.react: list[React]=[]
        print("before Starting Smart Thread")
        self.Thread: threading.Thread = threading.Thread(target=self.SmartRuner)
        self.Thread.start()
        self.Exchange=queue.Queue()
        self.Reaction=queue.Queue()
        self.Debug=queue.Queue()



    def QueSmartPop(self, queue: queue.Queue[Any]) -> Any:
        while queue.empty():
            pass
        return queue.get()


    def SmartRuner(self):
        '''

        :return: void
        '''
        print("Starting Smart Thread")




        while(True):

            packet=self.serial.read_packet()

            if packet is not None:

                if (packet["Stream"]=='E'):
                    print("E"+packet["data_str"])
                    self.Exchange.put(packet["data_str"])

                if (packet["Stream"] == 'R'):
                    print("R"+packet["data_str"])
                    self.Reaction.put(packet["data_str"])

                if (packet["Stream"] == 'D'):
                    print("D"+packet["data_str"])
                    self.Debug.put(packet["data_str"])






    def CheckReaction(self,code: int):
        for element in self.react:
            print(element)


    def send(self,type: str ,items: List[Item] ,resolver : React | None, chained: bool )-> int:
        '''
        Used to send a motor control command to the Esp32
        :param type: the Type of command "I" for "Interrupt","S" for "Sequential","R" for "Resolving","A" for "Asynchronous"
        :param items: list of items, see item class
        :param resolver: list of reactions used when a ticket is closed
        :param chained: if true function will hold till ticket is closed, if false function will not hold.
        :return: ticket number
        '''

        Ticket: int =-1

        if type not in ["I","S","R","A"]:
            return -1




        # todo
        # add restraints later



        # todo
        #request ticket
        self.serial.send_packet(0,"ReqTicket")
        ticket = self.QueSmartPop(self.Exchange)

        if (ticket == -1):
            return -1


        #todo
        #Format ticket

        self.serial.send_packet(0, "FormatTicket" + bytes.struct.pack('i', ticket) + type)
        # Load ticket

        for item in items:
            Strip: str = "LoadTicket"
            Strip += struct.pack('I', ticket).decode('latin-1')  # Unsigned int
            Strip += struct.pack('I', item.motor).decode('latin-1') #Unsigned int
            Strip += struct.pack('I', item.model).decode('latin-1') #Unsigned int
            Strip += struct.pack('?', len(item.command)).decode('latin-1') #Int of 1 byte
            Strip += item.command #string
            Strip += struct.pack('?', len(item.values)).decode('latin-1')  # Int of 1 byte
            for point in item.values:
                Strip += item.Press(point)
            self.serial.send_packet(0,  Strip)






        # todo
        # punch ticket
        self.serial.send_packet(0, "PunchTicket"+struct.pack('I', ticket).decode('latin-1'))


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