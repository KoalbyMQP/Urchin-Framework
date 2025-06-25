import serial
import keyboard
import time
import struct
import logging
from ESPSerial import EspConnect



# Initialize the serial connection using your custom EspConnect
ser = EspConnect()


# Setup logging
logging.basicConfig(
    filename='UrchinAPI.log',
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filemode='w'
)

logging.info("Starting Serial Communication Listener")

context = "null"
size = 1
number = 0

try:
    print("Waiting for serial data...")

    while True:
        # Simulate input request
        #request_str = "ReqTicket\0"
        #ser.write(request_str.encode('utf-8'))
        time.sleep(0.2)

        try:
            serial_data= ser.read(1025)
            decoded = serial_data.decode('utf-8').strip()
            print("Serial Data:", decoded)



        except serial.SerialException as e:
            logging.error(f"Serial error: {e}")
            print(f"Serial error occurred: {e}")
        except PermissionError as e:
            logging.error(f"Permission error: {e}")
            print(f"Permission error: {e}")

except KeyboardInterrupt:
    print("\n[!] Program stopped by user.")
    logging.info("Program stopped by user.")

finally:
    ser.close()
    logging.info("Serial connection closed.")
