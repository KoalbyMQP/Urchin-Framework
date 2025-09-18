import serial.tools.list_ports

for port in serial.tools.list_ports.comports():
    print(f"Device : {port.device}")
    print(f"  Name : {port.name}")
    print(f"  VID  : {hex(port.vid) if port.vid else 'Unknown'}")
    print(f"  PID  : {hex(port.pid) if port.pid else 'Unknown'}")
    print(f"  Description : {port.description}")
    print(f"  Manufacturer: {port.manufacturer}")
    print(f"  Serial No.  : {port.serial_number}")
    print()