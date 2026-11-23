# Quickstart: driving the motors from your app

**Overview:** quick guide on how to move the robot's motors with your own code. Follow it once to set up your computer or Pi, check that it
can talk to the motor circuit.


```
 Anticipated Pipeline: Mobile App ──MarshalType Item──► D-BOX over USB ──► ESP32 ──► motors
                    ◄──── reply "OK ..." / "ERR ..." ◄────
```

- **Commands and replies:** [`COMMAND_SHEET.md`](COMMAND_SHEET.md)

## What you need

- The ESP32 bench with DBoxMotor firmware and motors wired.
- Linux: the Pi, or Ubuntu under WSL on Windows.
- The repo, on branch `UV-Test`:
  ```
  git clone -b UV-Test https://github.com/KoalbyMQP/Urchin-Framework.git
  ```

## 1. One-time setup (Ubuntu or the Pi)

Tools:
```
sudo apt install -y cmake build-essential python3-dev
```

Build **DBOXpy** (the D-BOX Python module) by following
[`Clam/D-BOX/tools/README.md`](../Clam/D-BOX/tools/README.md), section
*From Python*. It ends up in `~/dbox`.

Build **marshaltype_py** (the MarshalType Python module) and put it next to it:
```
cd Urchin-Framework/Clam/MarshalType
```
```
cmake -B build && cmake --build build -j
```
```
cp build/python/marshaltype_py*.so ~/dbox/
```

**Windows/WSL only:** give the ESP32 to Ubuntu. Open Ubuntu first, then in
PowerShell (BUSID from `usbipd list`, "Silicon Labs CP210x"):
```
usbipd attach --wsl --busid X-X
```

## 2. Check the link

Power the motors, plug in the ESP32, then from the `Crab_API` folder:
```
sudo env PYTHONPATH=$HOME/dbox python3 src/marshal_test.py
```
Choose **1** (finds the motors), then **5** (synchronized move). Each step
should print `PASS`.

## Not done yet

- Tested from a laptop (Ubuntu/WSL), **not yet on the Pi / ZaraOS**.
- Stream letters and VPID meanings are **provisional** (see the command sheet).
- Bench has 2 motors (IDs 10 and 27), not the full robot.

