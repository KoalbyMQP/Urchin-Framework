import serial
import serial.tools.list_ports
import struct

from typing import Union, Any

from serial import Serial


class ESPSerial(object):
    def __init__(self,Debug: bool = False) -> None:
        '''

        '''
        self.START_MARKER: bytes = b'\x07'  # This is the bell character (\a)
        # The packet format for VPID (Byte), Stream (char), and data (1024s)
        self.PACKET_FORMAT: str = "<Bc1024s"
        self.PACKET_SIZE: int = struct.calcsize(self.PACKET_FORMAT)

        self.BoardTag: bytes = b'URCHIN'
        self.New: bool = True
        self.Debug: bool = Debug

        port: list[Serial] = self.find_esp32_ports()
        if len(port) == 0:
            raise ConnectionError("API did not find any open ports for Urchin firmware\n Insure ESP32 USB to UART serial number is \"urchin\"")

        self.buss: Serial = serial.Serial(port[0], 115200, timeout=1)

    def find_esp32_ports(self) -> list[str]:
        ports: list[Serial] = serial.tools.list_ports.comports()
        esp32_ports = []

        for port in ports:
            if (self.WhoaAreYou(port.device)):
                print(f"Found ESP32 port: {port.device}")
                esp32_ports.append(port.device)

        if len(esp32_ports) == 0:
            return []

        return esp32_ports



    def receive_packet(self):
        return self.receive_packet_internal(self.buss)

    def receive_packet_internal(self, bus: Serial) -> bytes:
        # Wait until the start marker is found
        if not self.find_start_marker_internal(bus):
            if self.New and self.Debug:
                print("Timeout while waiting for start marker (bell character).")
            self.New = False
            return None

        self.New = True

        # Now that the marker is found, read the actual packet data
        packet_data = bus.read(self.PACKET_SIZE)

        if len(packet_data) < self.PACKET_SIZE:
            print(f"Incomplete packet received ({len(packet_data)} bytes), expecting {self.PACKET_SIZE}.")
            print(packet_data.decode())

            return None

        return packet_data

    def find_start_marker(self) -> bool:
        return self.find_start_marker_internal(self.buss)

    def find_start_marker_internal(self, bus: Serial) -> bool:
        while True:
            byte = bus.read(1)
            if not byte:
                # Timeout occurred
                return False
            if byte == self.START_MARKER:
                # Marker found
                return True

    def send_packet(self, VPID: int, buff: bytes) -> None:
        self.send_packet_internal(VPID, buff, self.buss)

    def send_packet_internal(self, VPID: int, buff: bytes, bus: Serial) -> None:
        if not (0 <= VPID <= 255):
            raise ValueError("VPID must be an integer between 0 and 255")

        buff = buff.ljust(1024, b'\x00')[:1024]

        try:
            packed_data = struct.pack(self.PACKET_FORMAT,  VPID, b'N', buff)
            bus.write(self.START_MARKER + packed_data)

        except Exception as e:
            print("Error while packing or sending:", e)

    def read_packet(self)-> Union[dict[str,Any], None]:
        return self.read_packet_internal(self.buss)

    def read_packet_internal(self, bus: Serial) -> Union[dict[str,Any], None]:
        packet: bytes = self.receive_packet_internal(bus)

        if packet is None:
            return None

        try:
            VPID, Stream, data = struct.unpack(self.PACKET_FORMAT, packet)

            # Convert list[int] → bytes


            if self.Debug:
                decoded = data.decode("utf-8", errors="ignore").rstrip("\x00")
                print(f"VPID: {VPID}, Stream: {Stream}, Data: {decoded}")

            return {
                "VPID": VPID,
                "Stream": Stream,
                "data": data,
            }

        except struct.error:
            print("Failed to unpack packet.")
        except UnicodeDecodeError:
            print(f"UnicodeDecodeError, raw data: {repr(data)}")

        return None

    def close(self) -> None:
        self.buss.close()


    def WhoaAreYou(self, COM: str) -> bool:
        # Set const
        print("checking a com port")
        VPID: int = 0

        # Prep seral buss
        bus = serial.Serial(COM, 115200, timeout=1)

        # Prep message
        message = b'Validate'
        if self.Debug:
            print("sending Validate")
        self.send_packet_internal(VPID,message,bus)

        # reads until newline or timeout
        output = self.CheckForValidate(bus)
        bus.close()
        return output


    def CheckForValidate(self, bus) -> bool:
        packet = self.read_packet_internal(bus)
        if packet is None:
            return False
        if self.Debug:
            print("VPID:"+str(packet["VPID"]))
            print("Stream:" + str(packet["Stream"]))
            print("data:" + str(packet["data"]))
        if (packet["VPID"] == 0 and packet["Stream"] == b'D' and packet["data"].startswith(self.BoardTag) ):
            return True
        return False

