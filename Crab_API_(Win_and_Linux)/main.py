# This is a sample Python script.
import serial
import sys
# Press Shift+F10 to execute it or replace it with your code.
# Press Double Shift to search everywhere for classes, files, tool windows, actions, and settings.


def print_hi(name):
    # Use a breakpoint in the code line below to debug your script.
    print(f'Hi, {name}')  # Press Ctrl+F8 to toggle the breakpoint.


# Press the green button in the gutter to run the script.
if __name__ == '__main__':

    if(sys.platform=="win32"):
        ser = serial.Serial('COM4', 115200,timeout=1)


    if(sys.platform=="linux"):
        ser = serial.Serial('/dev/ttyUSB0', 115200)  # Linux

    ser.write("GetHealth".encode())
    while(True):
        print("|"+ser.readline().decode('utf-8').strip("")+"|")
    print_hi('PyCharm')

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
