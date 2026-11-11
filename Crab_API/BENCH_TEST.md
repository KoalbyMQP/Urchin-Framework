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
Connected on /dev/ttyUSB0: DBoxMotor v2 servos=219:0601
```

(`servos=` lists every servo the ESP32 spotted at power-on as `ID:model`;
`servos=none` just means run option 2.)

### 5. Work down the menu

| Option | What it checks | PASS looks like |
|---|---|---|
| **1) Check ESP32 link** | Laptop ↔ ESP32 over USB, 20 round trips | `PASS: 20/20 round trips OK` |
| **2) Check motors** | ESP32 ↔ servos: finds every servo on the bus (~10 s), prints each one's errors in plain English | `PASS: 1 servo found` + `no errors` |
| **3) Spin motor** | Everything, end to end | `d` spins clockwise (LED blue), `a` counter-clockwise (LED red), **space** stops, `q` goes back |
| **4) Move motor to an angle** | Everything, end to end | servo turns to the angle (LED green) |
| **5) Choose motor** | Which servo options 3 and 4 drive: one, or all at once (the default) | `Options 3 and 4 now drive motor 253 (DRS-0201)` |
| **6) Change a motor's ID** | Gives a servo a new ID, saved inside the servo | `PASS: motor 253 is now motor 12` |

The motor **always stops by itself** if the program is closed, crashes, or
the USB cable is pulled. It doesn't keep spinning without the laptop.

---

## More than one motor

Herkulex servos share one bus: chain them servo-to-servo (each servo has two
identical connectors), and only the first one connects to the ESP32. The
ESP32 tells them apart by **ID**, so every servo on the bus needs a different
one.

New servos all leave the factory with the same ID (DRS-0601: **219**,
DRS-0201: **253**), so add them **one at a time**:

1. Power off, plug in only the new servo, power on, run the test.
2. Option 2 (finds it), then option 6: give it a new ID (1–253) and write it
   on the servo with tape.
3. Power off, chain the next servo on, and repeat.

Then option 2 lists all of them, option 5 picks one (or all), and options 3
and 4 drive your pick. Each motor stops by itself if its keep-alive stops,
just like with one motor.

**Power:** every servo on the chain gets the same supply voltage, so it must
suit all of them. The DRS-0601 needed about **10 V** to move on our bench;
the DRS-0201 is rated 7–12 V but by default trips its over-voltage alarm
above **10.0 V** (red blinking LED, see below). Mixing the two, set the
supply just under 10 V (e.g. 9.8 V) and watch for alarms. Size the current
limit for all servos moving at once.

How it works (for developers): each servo is reached on D-BOX VPID = its ID;
VPID 0 means "all servos" (and the board itself). This numbering is
**provisional** until the team's stream/ID list is agreed; see the top of
`Urchin_Firmware/src/DBoxMotor/main.cpp`.

**No servos yet?** `Urchin_Firmware/tests/DBoxMotorSim/` runs the real
firmware on Linux against pretend servos, so you can try all of this without
hardware.

---

## Troubleshooting

| You see | Fix |
|---|---|
| `Can't find the DBOXpy module` | Build it (one-time setup, step 3) and keep the `PYTHONPATH=$HOME/dbox` part of the command. |
| `No serial ports found` | Use a **data** USB cable. On Windows, attach the board to WSL: `usbipd attach --wsl --busid 1-4`. |
| `/dev/ttyUSB0: no permission` | Run with `sudo` as shown, or `sudo usermod -aG dialout $USER` and log in again. |
| `FAIL: no ESP32 answered the D-BOX handshake` | Re-flash step 2. Press **EN/RST** on the ESP32, then run again. If you just pulled new code, rebuild DBOXpy **and** re-flash: the laptop/Pi and the ESP32 must run the same D-BOX version. |
| `Note: motor options need DBoxMotor firmware (this board runs DBoxEcho)` | The board has the echo test on it. Option 1 still works; for the rest, re-flash step 2. |
| Option 1: `FAIL: only n/20 round trips` | Different USB cable or port, straight into the laptop (no hub). |
| Flashing fails, or the USB connection drops whenever the ESP32 resets | The USB cable can't supply enough current: use a short, good-quality data cable. |
| Flashing: `Wrong boot mode detected` / `No serial data received` | Hold **BOOT** while it says `Connecting...`. |
| Option 2: `FAIL: no servo answered` | DC supply on? GND wire connected? Check servo TX → ESP32 GPIO16. Then try option 3: if it spins anyway, only the servo TX → GPIO16 wire is wrong. |
| Servo LED **blinks red as soon as the ESP32 is plugged in** | The GND wire between servo and ESP32 is missing or loose. Power off, fix it, power on. |
| Servo LED changes color (blue/red/green) but the shaft doesn't move | Supply voltage too low for that servo; the DRS-0601 needed about 10 V. |
| Option 2: `ERROR: INPUT VOLTAGE OUT OF RANGE` | Check the supply voltage; raise the current limit a little. A DRS-0201 trips this above 10.0 V by default. |
| Option 2 finds fewer servos than are plugged in | Two servos share an ID (both still on the factory ID?). Unplug all but one and use option 6, see *More than one motor*. |
| Option 3/4: `ERR NOSERVO` | The chosen motor stopped answering (unplugged, or its ID changed). Run option 2 again. |
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
* The test program still works with the older single-motor (v1) DBoxMotor
  firmware; options 5 and 6 then ask you to re-flash.
* `Urchin_Firmware/tests/DBoxMotorSim/`: the firmware running on Linux
  against simulated servos (see its README).
