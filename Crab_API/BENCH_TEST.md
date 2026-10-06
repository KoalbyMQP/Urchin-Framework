# Urchin Bench Test (laptop → ESP32 → motor)

Use this to check that the robot's motor path works, one link at a time,
from a laptop. It uses the team's D-BOX protocol under the hood, but you
don't need to know anything about it: pick an option from a menu and it
tells you **PASS** or **FAIL** plus what to check.

```
  laptop  ──USB (D-BOX)──►  ESP32  ──3 wires (Herkulex)──►  servo
          └─ option 1 ─┘           └──── option 2 ─────┘
          └────────────── options 3 & 4 (end to end) ──────────┘
```

If option 1 passes but option 2 fails, the problem is the ESP32 ↔ servo
wiring/power, not the laptop or USB.

---

## One-time setup

1. Install Python 3.11+ and VS Code with the **PlatformIO** extension.
2. In a terminal, from the `Crab_API` folder:
   ```
   pip install pyserial
   ```

---

## Steps

### 1. Wire the servo
Use the wiring diagram and power steps in
[`Urchin_Firmware/src/HerkulexTest/README.md`](../Urchin_Firmware/src/HerkulexTest/README.md)
(*Wiring* section). Same wiring, same rules: TX/RX cross over, shared GND,
servo power from the DC supply only. Leave the supply **off** for now.

### 2. Flash the DBoxMotor firmware (servo wires unplugged)
Unplug the 3 jumper wires from the ESP32, turn **Bluetooth off**, plug the
ESP32 into the laptop, and from the `Urchin_Firmware` folder run:

```
pio run -e DBoxMotor -t upload
```

Wait for `[SUCCESS]`. Then unplug USB.

> You only need to re-flash if the board was flashed with something else
> since (e.g. the HerkulexTest firmware).

### 3. Reconnect and power up
1. With everything unpowered, reconnect the 3 jumper wires.
2. Turn the DC supply **on**.
3. Plug the ESP32's USB back in.

### 4. Run the test program
Close any serial monitor first (only one program can use the port). Then,
from the `Crab_API` folder:

```
python src/bench_test.py
```

It finds the ESP32 by itself. If you have several devices, name the port:
`python src/bench_test.py COM5`.

You should see:

```
==== Urchin bench test ====
Looking for the ESP32 on: COM5
Connected on COM5: DBoxMotor v1 servo=<id>
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
| `No serial ports found` | Use a **data** USB cable (charge-only cables don't work). Try another USB port. |
| `COM5: in use by another program` | Close `pio device monitor`, VS Code's serial monitor, Arduino IDE, PuTTY. |
| `FAIL: no ESP32 answered the D-BOX handshake` | Re-flash step 2. Press **EN/RST** on the ESP32, then run again. |
| `Note: motor options need DBoxMotor firmware (this board runs DBoxEcho)` | The board has the echo test on it. Option 1 still works; for the rest, re-flash step 2. |
| Option 1: `FAIL: only n/20 round trips` | Different USB cable or port, straight into the laptop (no hub). Turn Bluetooth off. |
| Option 2: `FAIL: no servo answered` | DC supply on? GND wire connected? Check servo TX → ESP32 GPIO16. Then try option 3: if it spins anyway, only the servo TX → GPIO16 wire is wrong. |
| Option 2: `ERROR: INPUT VOLTAGE OUT OF RANGE` | Check the supply voltage; raise the current limit a little. |
| Option 2: `ERROR: OVERLOAD` | Something is blocking the shaft. Errors clear on the next spin/move. |
| Option 3/4: `FAIL: the ESP32 stopped answering` | USB cable came loose, or the ESP32 reset. Re-plug, run again. |

Still stuck? Copy the whole output and send it to the firmware team.

---

## For developers

* `src/dbox.py`: pure-Python D-BOX framing (byte-identical to Clam's C++;
  checked by `test/test_dbox.py` against frames from the C++ encoder).
* `src/bench_test.py`: this menu program.
* ESP32 side: `Urchin_Firmware/src/DBoxMotor/main.cpp` (the wire protocol is
  documented at the top of that file), built with `env:DBoxMotor`.
* Option 1 also works against `env:DBoxEcho` firmware, which echoes frames
  back using Clam's own C++ D-BOX code. That run is the hardware proof that
  the Python and C++ implementations agree on the wire.
