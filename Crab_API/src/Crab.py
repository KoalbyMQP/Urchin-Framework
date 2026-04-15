import queue
import threading
import struct
from ESPSerial import ESPSerial
from Types import Item
from enum import StrEnum
from typing import Union, Any, List
from Bridge import Bridge
from Reaction import Reaction
import struct
from typing import List, Tuple, Any
BRIDGEMaxName = 32  # must match C


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
        self.react: list[Reaction] = []
        self.Alive:bool = True
        self.dev:bool = dev

        self.Exchange: queue.Queue[Any] = queue.Queue()
        self.Reaction: queue.Queue[Any] = queue.Queue()
        self.Debug: queue.Queue[Any] = queue.Queue()

        self.OurTickets: list[int] = []

        self.Thread: threading.Thread = threading.Thread(target=self._SmartRuner)
        self.Thread.start()

    def _QueSmartPop(self, queue: queue.Queue[Any],Format: str) -> Any:
        """
           Blocking pop from queue and unpack according to Format.
           Format should be a struct format string like '<I' or the single-letter 'S' for full-string.
           This function is defensive: converts list->bytes, checks lengths, and prints diagnostics on error.
           """
        buf = queue.get(block=True)

        # Defensive normalization
        if isinstance(buf, list):
            buf = bytes(buf)
        elif isinstance(buf, bytearray):
            buf = bytes(buf)
        elif not isinstance(buf, (bytes, bytearray)):
            try:
                buf = bytes(buf)
            except Exception:
                raise TypeError(f"Queue payload is not bytes-like: {type(buf)}")

        # Normalize format to string
        fmt_s = Format.decode() if isinstance(Format, (bytes, bytearray)) else str(Format)

        # If user asked for string
        if fmt_s.upper() == "S":
            return buf.decode("utf-8", errors="ignore").rstrip("\x00")

        # Unpack fixed-width binary
        size = struct.calcsize(fmt_s)
        if len(buf) < size:
            # helpful debug: show start of buffer so we can debug quickly
            print(f"_QueSmartPop: buffer too small for '{fmt_s}' (need {size}, got {len(buf)})")
            print("  buffer hex:", buf.hex())
            raise ValueError(f"buffer too small for format {fmt_s}: need {size}, got {len(buf)}")

        return struct.unpack(fmt_s, buf[:size])[0]

    def _SmartRuner(self) -> None:
        '''

        :return: void
        '''

        while self.Alive:

            packet = self.serial.read_packet()

            if packet is not None:



                if (packet["Stream"] == b'E'):
                    self.Exchange.put(packet["data"])

                if (packet["Stream"] == b'R'):
                    print("got react")
                    self.Reaction.put(packet["data"])

                    parsed = self.parse_reaction_packet(packet["data"])

                    values = [v for _, v in parsed["values"]]

                    expected_params = self._values_to_expected_params(parsed["values"])

                    for react in self.react:
                        if react.CompareIncomeing(
                                TicketNum=parsed["ticket"],
                                Joint=parsed["joint"],
                                CommandName=parsed["command_name"],
                                CommandCode=parsed["code"],
                                expected_params=expected_params
                        ):
                            try:
                                react.Function(*values)
                            except Exception as e:
                                print(f"Reaction execution failed: {e}")


                if (packet["Stream"] == b'D'):
                    self.Debug.put(packet["data"])

        self.serial.close()

    def _CheckReaction(self,code: int) -> None:
        for element in self.react:
            print(element)

    def parse_reaction_packet(self, buffer: bytes):
        """
        Parse raw packet from SendReaction into structured Python data.
        """

        header_fmt = f"<II{BRIDGEMaxName}sBB"
        header_size = struct.calcsize(header_fmt)

        ticket, code, joint_raw, cmd_len, values_len = struct.unpack(
            header_fmt, buffer[:header_size]
        )

        joint = joint_raw.split(b'\x00', 1)[0].decode()

        offset = header_size

        command_name = buffer[offset:offset + cmd_len].decode()
        offset += cmd_len

        values = []

        for _ in range(values_len):
            base = offset

            # Read type
            value_type = chr(buffer[base])

            # Read full 4-byte union (always at +4)
            raw = buffer[base + 4: base + 8]

            if value_type == 'i':
                value = struct.unpack("<i", raw)[0]

            elif value_type == 'f':
                value = struct.unpack("<f", raw)[0]

            elif value_type == 'b':
                value = struct.unpack("<b", raw[:1])[0]

            else:
                raise ValueError(f"Unknown type: {value_type}")

            values.append((value_type, value))

            offset += 8  # move to next struct

        return {
            "ticket": ticket,
            "code": code,
            "joint": joint,
            "command_name": command_name,
            "values": values
        }

    def _values_to_expected_params(self, values):
        type_map = {
            'i': int,
            'f': float,
            'b': int,  # or bool if you prefer
        }

        return [
            (type_map[t], f"param{idx}")
            for idx, (t, _) in enumerate(values)
        ]


    def send(self, type: TicketType, items: List[Item], resolver: Union[Reaction, None], chained: bool) -> int:
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
        Error: int = self._QueSmartPop(self.Exchange, "<i") # int
        if (Error == 1):
            return -1

        # Receve Ticket
        ticket: int = self._QueSmartPop(self.Exchange, "<I") # unsigned int

        self.OurTickets.append(ticket)
        if resolver is not None:
            for rect in resolver:
                rect.SetTicket(ticket)
                self.react.append(rect)



        print("got here")
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
        self.serial.send_packet(0,("GetHealth" + type).encode('utf-8'))

        #if (type == "AckCheck"):

           # AckCheckRead = self.serial.readline()
            #while (not AckCheckRead == b'AckCheck:done'):










    def close(self) -> None:
        self.Alive = False
        self.Thread.join()

class CrabError(Exception):
    """A simple custom exception."""
    pass
