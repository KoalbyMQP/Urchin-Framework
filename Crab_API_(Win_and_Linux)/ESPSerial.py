import serial
import serial.tools.list_ports

def EspConnect():
    ser = serial.Serial( "com5", 115200, timeout=1)
    return ser


def find_esp32_ports():
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
