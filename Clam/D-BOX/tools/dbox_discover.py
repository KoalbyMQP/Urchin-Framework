"""
dbox_discover.py: the Python version of tools/dbox_discover.cpp.

Finds the robot's ESP32 with D-BOX discovery through the DBOXpy bindings
(the same C++ D-BOX code, called from Python), then sends "Info" and prints
the reply. This is how the Pi's Python code should talk D-BOX.

    python3 dbox_discover.py                  # try every serial port
    python3 dbox_discover.py /dev/ttyUSB0     # try only these ports

Needs the DBOXpy module built (see tools/README.md) and importable, e.g.
run from the folder that contains DBOXpy*.so, or set PYTHONPATH.
"""

import sys

import DBOXpy as d


def main(argv):
    cfg = d.DconfigUart("Urchin", "", 115200, 20)
    disco = d.UartDiscovery(True, cfg)
    port = d.LinuxSerialPort(disco)
    proto = d.DBOXprotocol(port, "/dev/null")

    ports = argv[1:] or port.find()
    if not ports:
        print("No serial ports found. Is the ESP32 plugged in (and attached to WSL)?")
        return 2

    found = None
    for p in ports:
        print(f"Trying {p:<22} ... ", end="", flush=True)
        if disco.active_discover_single(p, disco.attempts_per_port):
            print(f'"{cfg.identity}" answered')
            found = p
            break
        print("no answer")

    if not found:
        print("\nRobot not found on any port.")
        return 1

    print(f"\nFound the robot on {found}")
    if not port.openraw(found, cfg.baud, cfg.timeout):
        print("...but could not reopen it.")
        return 1

    proto.send(d.DBOX(0, "N", "Info"))
    print('Sent:  N "Info"')

    reply = d.DBOX()
    if proto.receive(reply, 2000):
        print(f'Got:   {reply.stream} "{reply.text}"')
        return 0
    print("No reply to Info within 2 s.")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
