
import serial
import struct



START_MARKER = b'\x07'  # This is the bell character (\a)
# The packet format for VPID (Byte), Stream (char), and data (1024s)
PACKET_FORMAT = "<Bc1024s"
PACKET_SIZE = struct.calcsize(PACKET_FORMAT)

# Ensure you are using the correct COM port and baud rate
ser = serial.Serial('COM3', 115200, timeout=1)

def find_start_marker(ser, marker):
    while True:
        byte = ser.read()
        if not byte:
            # Timeout occurred
            return False
        if byte == marker:
            # Marker found
            return True

def read_packet(ser):
    # Wait until the start marker is found
    if not find_start_marker(ser, START_MARKER):
        print("Timeout while waiting for start marker (bell character).")
        return None

    # Now that the marker is found, read the actual packet data
    packet_data = ser.read(PACKET_SIZE)

    if len(packet_data) < PACKET_SIZE:
        print(f"Incomplete packet received ({len(packet_data)} bytes), expecting {PACKET_SIZE}.")
        return None

    return packet_data

# --- Main Loop ---
try:
    while True:
        packet = read_packet(ser)

        if packet:
            try:

                VPID, Stream, raw_data = struct.unpack(PACKET_FORMAT, packet)

                data_str = raw_data.decode('utf-8').rstrip('\x00')

                print(f"VPID: {VPID}, Stream: {Stream}, Data: {data_str}")
                #print(len(data_str))
            except struct.error:
                print("Failed to unpack packet.")
            except UnicodeDecodeError:
                print(f"UnicodeDecodeError, raw data: {repr(raw_data)}")
finally:
    print("\nClosing serial port.")
    ser.close()