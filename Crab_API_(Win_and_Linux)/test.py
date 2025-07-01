import serial
import struct

ser = serial.Serial('COM3', 115200, timeout=5)

packet_size = struct.calcsize("<BB1024s")


def read_exact(ser, n):
    buf = b''
    while len(buf) < n:
        chunk = ser.read(n - len(buf))
        if not chunk:
            # No data received, possibly timeout
            break
        buf += chunk
    return buf

buffer = b''
while True:
    byte = ser.read()  # Read one byte
    buffer += byte
    if b'<<StartStart>>' in buffer:
        break

while True:
    data = read_exact(ser, packet_size)
    if len(data) != packet_size:
        print(f"Incomplete packet received ({len(data)} bytes), skipping")
        continue

    VPID, Stream, raw_data = struct.unpack("<BB1024s", data)
    try:
        data_str = raw_data.decode('utf-8').rstrip('\x00')
    except UnicodeDecodeError:
        data_str = repr(raw_data)

    print(f"VPID: {VPID}, Stream: {Stream}, Data: {data_str}")
