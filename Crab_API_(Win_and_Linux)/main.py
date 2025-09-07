# This is a sample Python script.
import serial
import sys
from Smart import *
# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.

from ESPSerial import ESPSerial


# Press the green button in the gutter to run the script.
if __name__ == '__main__':

    #if(sys.platform=="win32"):
    #    ser = serial.Serial('COM4', 115200,timeout=1)


    #if(sys.platform=="linux"):
    #    ser = serial.Serial('/dev/ttyUSB0', 115200)  # Linux

    ser = ESPSerial()

    try:

        while True:
           ser.send_packet(0,"GetHealth")
           ser.read_packet()


    finally:
        print("\nClosing serial port.")
        ser.close()

