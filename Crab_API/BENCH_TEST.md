# Urchin Bench Test (laptop/Pi → ESP32 → motor)

Use this to check that the robot's motor path works, one link at a time,
from a laptop or the Raspberry Pi. It uses the team's D-BOX protocol under
the hood, but you don't need to know anything about it: pick an option from a
menu and it tells you **PASS** or **FAIL** plus what to check.

```
  laptop/Pi ──USB (D-BOX)──►  ESP32  ──3 wires (Herkulex)──►  servo
            └─ option 1 ─┘           └──── option 2 ─────┘
            └────────────── options 3 & 4 (end to end) ──────────┘
```

If option 1 passes but option 2 fails, the problem is the ESP32 ↔ servo
wiring/power, not the laptop or USB.

The test program runs on **Linux**: the Raspberry Pi, or Windows through
**WSL** (Ubuntu). It talks D-BOX through `DBOXpy`, the Python bindings of the
team's C++ D-BOX code, so it exercises the exact code the robot uses.
Flashing the ESP32 still happens from Windows (PlatformIO).

---

## One-time setup

1. VS Code with the **PlatformIO** extension (for flashing).
2. **Windows only:** WSL with Ubuntu (`wsl --install -d Ubuntu` in PowerShell),
   and usbipd to hand the ESP32 to WSL:
   ```
   winget install --exact dorssel.usbipd-win
   ```
   Plug in the ESP32, run `usbipd list`, note the BUSID of
   "Silicon Labs CP210x" (e.g. `1-4`), then once, in an **Administrator**
   PowerShell: `usbipd bind --busid 1-4`
3. Build `DBOXpy` (in Ubuntu, or on the Pi), from `Clam/D-BOX`, following
   [`Clam/D-BOX/tools/README.md`](../Clam/D-BOX/tools/README.md), section
   *From Python*. It ends up in `~/dbox`.

---

## Steps

### 1. Wire the servo
Use the wiring diagram and power steps in
[`Urchin_Firmware/src/HerkulexTest/README.md`](../Urchin_Firmware/src/HerkulexTest/README.md)
(*Wiring* section). Same wiring, same rules: TX/RX cross over, shared GND,
servo power from the DC supply only. Leave the supply **off** for now.

### 2. Flash the DBoxMotor firmware (servo wires unplugged, from Windows)
Unplug the 3 jumper wires from the ESP32 and plug the ESP32 into the laptop
with a **short, good-quality data cable**. From the `Urchin_Firmware` folder:

```
pio run -e DBoxMotor -t upload
```

When it says `Connecting...`, hold the ESP32's **BOOT** button until
`Writing at ...` appears (some boards don't enter download mode by themselves).
Wait for `Hash of data verified` and `[SUCCESS]`. Then unplug USB.

If the board was attached to WSL, give it back to Windows first:
`usbipd detach --busid 1-4`.

> You only need to re-flash if the board was flashed with something else
> since (e.g. the HerkulexTest firmware).

### 3. Reconnect and power up
1. With everything unpowered, reconnect the 3 jumper wires.
2. Turn the DC supply **on**.
3. Plug the ESP32's USB back in.
4. **Windows:** open Ubuntu, then in PowerShell: `usbipd attach --wsl --busid 1-4`.
   In Ubuntu, `ls /dev/ttyUSB*` should show `/dev/ttyUSB0`.

### 4. Run the test program
Close any serial monitor first (only one program can use the port). Then, in
Ubuntu or on the Pi, from the `Crab_API` folder:

```
sudo env PYTHONPATH=$HOME/dbox python3 src/bench_test.py
```

It finds the ESP32 by itself (D-BOX discovery). If you have several devices,
name the port: `... src/bench_test.py /dev/ttyUSB0`.

You should see:

```
==== Urchin bench test ====
Looking for the ESP32 on: /dev/ttyUSB0
Connected on /dev/ttyUSB0: DBoxMotor v1 servo=<id>
```

### 5. Work down the menu

| Option | What it checks | PASS looks like |
|---|---|---|
| **1) Check ESP32 link** | Laptop ↔ ESP32 over USB, 20 round trips | `PASS: 20/20 round trips OK` |
| **2) Check motor** | ESP32 ↔ servo; prints servo errors in plain English | `PASS: servo found (ID n)` + `no errors` |
| **3) Spin motor** | Everything, end to end | `d` spins clockwise (LED blue), `a` counter-clockwise (LED red), **space** stops, `q` goes back |
| **4) Move motor to an angle** | Everything, end to end | servo turns to the angle (LED green) |

The motor **always stops by itself** if the program is closed, crashes, or
the USB cable is pulled. It doesn't keep spinning without the laptop.

---

## Troubleshooting

| You see | Fix |
|---|---|
| `Can't find the DBOXpy module` | Build it (one-time setup, step 3) and keep the `PYTHONPATH=$HOME/dbox` part of the command. |
| `No serial ports found` | Use a **data** USB cable. On Windows, attach the board to WSL: `usbipd attach --wsl --busid 1-4`. |
| `/dev/ttyUSB0: no permission` | Run with `sudo` as shown, or `sudo usermod -aG dialout $USER` and log in again. |
| `FAIL: no ESP32 answered the D-BOX handshake` | Re-flash step 2. Press **EN/RST** on the ESP32, then run again. |
| `Note: motor options need DBoxMotor firmware (this board runs DBoxEcho)` | The board has the echo test on it. Option 1 still works; for the rest, re-flash step 2. |
| Option 1: `FAIL: only n/20 round trips` | Different USB cable or port, straight into the laptop (no hub). |
| Flashing fails, or the USB connection drops whenever the ESP32 resets | The USB cable can't supply enough current: use a short, good-quality data cable. |
| Flashing: `Wrong boot mode detected` / `No serial data received` | Hold **BOOT** while it says `Connecting...`. |
| Option 2: `FAIL: no servo answered` | DC supply on? GND wire connected? Check servo TX → ESP32 GPIO16. Then try option 3: if it spins anyway, only the servo TX → GPIO16 wire is wrong. |
| Option 2: `ERROR: INPUT VOLTAGE OUT OF RANGE` | Check the supply voltage; raise the current limit a little. |
| Option 2: `ERROR: OVERLOAD` | Something is blocking the shaft. Errors clear on the next spin/move. |
| Option 3/4: `FAIL: the ESP32 stopped answering` | USB cable came loose, or the ESP32 reset. Re-plug (and re-attach to WSL), run again. |

Still stuck? Copy the whole output and send it to the firmware team.

---

## For developers

* D-BOX comes from `DBOXpy`, built from `Clam/D-BOX/src/DBOX_bindings.cpp`
  over the C++ D-BOX code (protocol, `UartDiscovery`, `LinuxSerialPort`).
  There is no separate Python implementation of D-BOX any more (the old
  `src/dbox.py` was removed so the rules live in one place).
* `src/bench_test.py`: this menu program. The connection is found with D-BOX
  discovery (`UartDiscovery`, identity `"Urchin"`).
* ESP32 side: `Urchin_Firmware/src/DBoxMotor/main.cpp` (the wire protocol is
  documented at the top of that file), built with `env:DBoxMotor`.
* Option 1 also works against `env:DBoxEcho` firmware, which echoes frames back.
