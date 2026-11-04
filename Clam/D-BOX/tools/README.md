# dbox_discover: find the ESP32 with D-BOX discovery (real hardware)

Runs D-BOX discovery over the real serial ports, prints which one the robot
is on, then sends `N "Info"` and prints the reply. Linux only: Raspberry Pi,
or Windows through WSL.

ESP32 firmware: `DBoxMotor` (replies `E "OK DBoxMotor v1 ..."`) or
`DBoxEcho` (echoes, so you get `N "Info"` back). Both answer the handshake.

Expected output:

    Trying /dev/ttyUSB0           ... "Urchin" answered

    Found the robot on /dev/ttyUSB0
    Sent:  N "Info"
    Got:   E "OK DBoxMotor v1 servo=broadcast"

## Build (WSL Ubuntu or Raspberry Pi OS), from `Clam/D-BOX`

    sudo apt install -y g++ libboost-dev
    g++ -std=c++20 -Iinclude tools/dbox_discover.cpp \
        src/DBOX-Protocol/DBOXprotocol.cpp src/Discovery/UartDiscovery.cpp \
        src/Transport-Interface/IPort.cpp \
        src/Transport-Interface/UART/Linux/linux-sys-seral.cpp \
        -o dbox_discover

## Run on a Raspberry Pi

Plug the ESP32 into the Pi's USB, then `./dbox_discover`.
If it says permission denied: `sudo usermod -aG dialout $USER`, log out and in.

## Run on Windows through WSL

WSL can't see USB devices by default; `usbipd` hands the ESP32 to it.

1. One time, in PowerShell: `winget install --exact dorssel.usbipd-win`
2. Plug in the ESP32. In PowerShell, `usbipd list` and note the BUSID of
   "Silicon Labs CP210x" (e.g. `2-3`).
3. Once per board, in an **Administrator** PowerShell: `usbipd bind --busid 2-3`
4. Each time you plug it in, with Ubuntu open: `usbipd attach --wsl --busid 2-3`
   (COM5 disappears from Windows while attached.)
5. In Ubuntu: `ls /dev/ttyUSB*` should show `/dev/ttyUSB0`. Then run
   `sudo ./dbox_discover` (or add yourself to `dialout` as above).
6. To give the board back to Windows (e.g. to flash it):
   `usbipd detach --busid 2-3`

---

# From Python: DBOXpy + dbox_discover.py

`DBOXpy` is the Python module built from `src/DBOX_bindings.cpp`. It calls
the same C++ D-BOX code (protocol, discovery, Linux serial port), so Python on
the Pi gets the exact same behavior as the C++ tools. `tools/dbox_discover.py`
is the Python version of `dbox_discover`.

## Build the module (WSL Ubuntu or Raspberry Pi OS), from `Clam/D-BOX`

    sudo apt install -y g++ libboost-dev python3-dev python3-pybind11
    mkdir -p ~/dbox
    g++ -std=c++20 -O1 -shared -fPIC $(python3 -m pybind11 --includes) -Iinclude \
        src/DBOX_bindings.cpp src/DBOX-Protocol/DBOXprotocol.cpp \
        src/Discovery/UartDiscovery.cpp src/Transport-Interface/IPort.cpp \
        src/Transport-Interface/UART/UARTSeralPort.cpp \
        src/Transport-Interface/UART/Linux/linux-sys-seral.cpp \
        -o ~/dbox/DBOXpy$(python3-config --extension-suffix)

(Gabe's CMake builds the same module as target `D-BOX_py`.)

## Test it (no hardware)

    PYTHONPATH=~/dbox python3 test/Python/test_DBOXpy.py -v

## Run it on the ESP32 (attached to WSL as /dev/ttyUSB0, see above)

    sudo env PYTHONPATH=$HOME/dbox python3 tools/dbox_discover.py

Same expected output as the C++ version.
