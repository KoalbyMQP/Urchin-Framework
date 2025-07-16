import queue
import threading

from ESPSerial import *
from Types import *


class Smart:
    def __init__(self,serial: str)->None:
        #todo
        #start the listener loop
        self.react=[]

        self.Thread = threading.Thread(target=self.SmartRuner)

        self.Exchange=queue.Queue()
        self.Reaction=queue.Queue()
        self.Debug=queue.Queue()

        self.serial=serial



    def SmartRuner(self):
        print("Starting Smart Thread")
        serial=ESPSerial()



        while(True):

            port=serial.read_packet()

            if (port["Stream"]=='E'):
                print("E"+port["data_str"])
                self.Exchange.put(port["data_str"])

            if (port["Stream"] == 'R'):
                print("R"+port["data_str"])
                self.Reaction.put(port["data_str"])

            if (port["Stream"] == 'D'):
                print("D"+port["data_str"])
                self.Debug.put(port["data_str"])




    def send(self,type: str ,items: List[Item] ,resolver : React, chained: bool )-> int:
        '''
        Used to send a motor control command to the Esp32
        :param type: the Type of command "Interrupt","Sequential","Resolving","asynchronous"
        :param items: list of items, see item class
        :param resolver: list of reactions used when a ticket is closed
        :param chained: if true function will hold till ticket is closed, if false function will not hold.
        :return: ticket number
        '''

        # deprecated due to Typing
        # if not isinstance(type,str):
        #     raise TypeError('type must be str')
        # if type not in ["Interrupt","Sequential","Resolving","asynchronous"]:
        #     raise TypeError("Type must be one of Interrupt , Sequential , Resolving , asynchronous")
        #
        # if not isinstance(items,list):
        #     raise TypeError("items must be a list")
        # if not all(isinstance(item, Items) for item in items):
        #     raise TypeError("all elements in items must be \"Items\"")

        # todo
        # add restraints later

        # deprecated due to Typing
        # if not isinstance(resolver,list):
        #     raise TypeError("resolver must be a list")
        # if not all(isinstance(hold,Reaction) for hold in resolver):
        #     raise TypeError("all elements in resolver must be a\"Reaction\"")
        #
        # if not isinstance(chained,bool):
        #     raise TypeError("chained must be a bool")

        # todo
        #request ticket


        #todo
        #punch ticket


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

