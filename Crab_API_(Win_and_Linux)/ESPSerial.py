import serial
import serial.tools.list_ports
import struct


class ESPSerial(object):
    START_MARKER = b'\x07'  # This is the bell character (\a)
    # The packet format for VPID (Byte), Stream (char), and data (1024s)
    PACKET_FORMAT = "<Bc1024s"
    PACKET_SIZE = struct.calcsize(PACKET_FORMAT)

    def __init__(self):
        self.serial = serial.Serial(self.find_esp32_ports()[0], 115200, timeout=1)



    def find_esp32_ports(self):
        ports = serial.tools.list_ports.comports()
        esp32_ports = []

        for port in ports:


            if ("CP210" in port.description or
                    "CH340" in port.description or
                    "Silicon Labs" in port.manufacturer or
                    "USB Serial" in port.description or
                    "FTDI" in port.description or
                    "UART" in port.description):
                esp32_ports.append((port.device, port.description))
        if len(esp32_ports) == 0:
            return False

        return esp32_ports[0]


    def receive_packet(self):
        # Wait until the start marker is found
        if not self.find_start_marker():
            print("Timeout while waiting for start marker (bell character).")
            return None

        # Now that the marker is found, read the actual packet data
        packet_data = self.serial.read(self.PACKET_SIZE)

        if len(packet_data) < self.PACKET_SIZE:
            print(f"Incomplete packet received ({len(packet_data)} bytes), expecting {self.PACKET_SIZE}.")
            print(packet_data.decode())

            return None

        return packet_data




    def find_start_marker(self):
        while True:
            byte = self.serial.read()
            if not byte:
                # Timeout occurred
                return False
            if byte == self.START_MARKER:
                # Marker found
                return True

   # def send_packet(self, VPID, buff):
   #     if isinstance(buff, str):
   #         buff = buff.encode('utf-8')
    #    buff = buff.ljust(1024, b'\x00')[:1024]
    #    packed_data = struct.pack("<cBB1024s", b'\a', VPID, ord('N'), buff)
   #    self.serial.write(packed_data)

    def send_packet(self, VPID, buff):
        if not isinstance(VPID, int) or not (0 <= VPID <= 255):
            raise ValueError("VPID must be an integer between 0 and 255")

        if isinstance(buff, str):
            buff = buff.encode('utf-8')

        buff = buff.ljust(1024, b'\x00')[:1024]

        try:
            packed_data = struct.pack("<cBB1024s", b'\a', VPID, ord('N'), buff)
            self.serial.write(packed_data)

            #print(struct.calcsize("<cBB1024s"))
            #print("Decode:  "+packed_data.decode())
        except Exception as e:
            print("Error while packing or sending:", e)


    def read_packet(self):


        packet = self.receive_packet()

        if packet:
            try:

                VPID, Stream, raw_data = struct.unpack(self.PACKET_FORMAT, packet)

                data_str = raw_data.decode('utf-8').rstrip('\x00')

                print(f"VPID: {VPID}, Stream: {Stream}, Data: {data_str}")
                # print(len(data_str))
            except struct.error:
                print("Failed to unpack packet.")
            except UnicodeDecodeError:
                print(f"UnicodeDecodeError, raw data: {repr(raw_data)}")

            return [VPID,Stream,data_str]


    def close(self):
        self.serial.close()