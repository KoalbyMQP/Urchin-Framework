import queue
import threading
import struct
from ESPSerial import ESPSerial
from Types import Item, React
from enum import StrEnum
from typing import Union, Any, List
from Bridge import Bridge

class TicketType(StrEnum):
    Sequential:str = "S"
    Resolving:str = "R"
    Asynchronous:str = "A"
    Interrupt:str = "I"

class Crab:
    def __init__(self, bridge:str, dev: bool = False) -> None:

        self.serial = ESPSerial(dev)
        self.bridge = Bridge(bridge,self.serial)
        self.bridge.SendAll()
        self.react: list[React] = []
        self.Alive:bool = True
        self.dev:bool = dev

        self.Exchange: queue.Queue[Any] = queue.Queue()
        self.Reaction: queue.Queue[Any] = queue.Queue()
        self.Debug: queue.Queue[Any] = queue.Queue()

        self.Thread: threading.Thread = threading.Thread(target=self._SmartRuner)
        self.Thread.start()

    def _QueSmartPop(self, queue: queue.Queue[Any],Format: str) -> Any:
        while queue.empty():
            pass
        buffer: str = queue.get()
        if Format == b'S':
            return bytes(buffer).decode('utf-8').rstrip('\x00')
        buffer: bytes = bytes(buffer[:(struct.calcsize(Format))])
        return struct.unpack(Format, buffer)[0]

    def _SmartRuner(self) -> None:
        '''

        :return: void
        '''

        while self.Alive:

            packet = self.serial.read_packet()

            if packet is not None:

                if (packet["Stream"] == b'E'):
                    self.Exchange.put(packet["data_str"])

                if (packet["Stream"] == b'R'):
                    self.Reaction.put(packet["data_str"])

                if (packet["Stream"] == b'D'):
                    self.Debug.put(packet["data_str"])

        self.serial.close()

    def _CheckReaction(self,code: int) -> None:
        for element in self.react:
            print(element)

    def send(self, type: TicketType, items: List[Item], resolver: Union[React, None], chained: bool) -> int:
        '''
        Used to send a motor control command to the Esp32
        :param type: the Type of command "I" for "Interrupt","S" for "Sequential","R" for "Resolving","A" for "Asynchronous"
        :param items: list of items, see item class
        :param resolver: list of reactions used when a ticket is closed
        :param chained: if true function will hold till ticket is closed, if false function will not hold.
        :return: ticket number
        '''

        # todo
        # Add restraints later




        # Format ticket
        if self.dev:
            print("sending:" + "FormatTicket" + type)
        self.serial.send_packet(0, b"FormatTicket" + type.encode('utf-8'))

        # Check Error
        Error: int = self._QueSmartPop(self.Exchange, "<i")
        if (Error == 1):
            return -1

        # Receve Ticket
        ticket: int = self._QueSmartPop(self.Exchange, "<I")




        # Load ticket
        for item in items:
            Strip: str = b"LoadTicket"
            Strip += struct.pack(b'I', ticket)  # Unsigned int
            Strip += struct.pack(b'50s', item.joint.encode('utf-8')) # String of max Size 50
            Strip += struct.pack(b'B', len(item.command)) #Int of 1 byte
            Strip += struct.pack(b'B', len(item.values))  # Int of 1 byte
            Strip += item.command.encode('utf-8') #string
            for point in item.values:
                Strip += item.Press(point)
            self.serial.send_packet(0, Strip)
        # Punch ticket
        self.serial.send_packet(0, b"PunchTicket" + struct.pack('I',ticket))
        # todo
        # Add resolver to the self.react
        # todo
        return ticket

    def CloseTicket(self,ticket: int) -> None:
        print('Closing Ticket')
        # todo
        # send "CloseTicket" command to esp32

    def TicketInfo(self,ticket: int) -> None:
        print('Ticket Info')
        # todo
        # request ticket status

    def GetHealth(self,type: str) -> str:
        if not isinstance(type,str):
            raise TypeError("type must be str")

        if type not in ["TotalRam","FreeRam","CPU","AckCheck"]:
            raise TypeError("Type must be one of TotalRam,FreeRam,CPU,AckCheck")

        # todo
        # send "GetHealth" to esp32 along with the type
        self.serial.write(("GetHealth" + type).encode('utf-8'))

        return self.serial.readline()

    def close(self) -> None:
        self.Alive = False
        self.Thread.join()
