# Herkulex Servo Bench Test

Use this to check that one Herkulex servo (DRS-0601) can be driven by an
ESP32 over UART. No other part of the robot (Pi, D-BOX, Urchin app) is
involved. Hold a key and the servo spins.

You don't need embedded experience. Follow the steps **in order**. Most
"it worked yesterday but not today" problems come from skipping step 3.

---

## What you need

| Item | Notes |
|---|---|
| ESP32 dev board (ESP32-WROOM-32 / "esp32dev") | If the metal can says **WROVER**, stop and ask: pins 16/17 are not free on that module. |
| USB cable for the ESP32 | Must be a **data** cable. Charge-only cables power the board but can't flash it. |
| Herkulex DRS-0601 servo with its 4-pin cable | Pins: **GND, VDD, TX, RX** |
| Bench DC power supply | Set the voltage to what's on the servo's label/datasheet **before** connecting it. |
| 3 jumper wires | For TX, RX and GND to the ESP32. |
| A PC with VS Code + the **PlatformIO** extension | Run commands from the PlatformIO terminal, inside the `Urchin_Firmware` folder. |

---

## Wiring

```
   Bench DC supply                Herkulex servo                 ESP32
   ---------------                --------------                 -----
        (+)  ------------------->  VDD
        (-)  ------------------->  GND  <----------------------  GND   (shared ground!)
                                   RX   <----------------------  GPIO17 (TX)
                                   TX   ---------------------->  GPIO16 (RX)
```

* **TX goes to RX and RX goes to TX.** The wires cross over.
* **The ground wire between the servo and the ESP32 is required.** Without
  it the servo gets garbage. Sometimes it works anyway, which makes the
  problem look random.
* **Never connect the servo's VDD to the ESP32.** The servo gets its power
  only from the DC supply. The ESP32 gets its power only from USB.

---

## Steps

### 1. Set up the supply (output OFF)
Set the voltage to the servo's rated value and keep the output **off**.
If the supply has a current limit, set it to about 1-2 A. That's enough to
spin with no load, and it protects the servo if something is mis-wired.

### 2. Disconnect the servo from the ESP32
Unplug the 3 jumper wires (TX, RX, GND) from the ESP32.

> **Why:** with the servo wired up, flashing fails with
> `Failed to connect to ESP32: No serial data received`. Flash first,
> then wire up. This is the single most common setup failure.

### 3. Flash the test firmware (USB only)
Plug the ESP32 into the PC with USB, then run:

```
pio run -e HerkulexTest -t upload
```

Wait for `[SUCCESS]`. You don't need to set any environment variables;
the build sets itself up.

*Turn Bluetooth off before flashing.* On our test laptop, uploads failed with Bluetooth on and passed every time with it off.

*If the upload can't connect:* hold the board's **BOOT** button, start the
upload command again, and release BOOT once you see `Connecting...`
followed by `Writing at 0x...`.

### 4. Unplug USB, then wire the servo
With **everything unpowered** (USB unplugged, supply output off), connect
the 3 jumper wires as shown in the wiring diagram. Double-check that TX/RX
cross over and that GND is connected.

### 5. Power the servo, then the ESP32
1. Turn the DC supply output **on**. The servo's LED usually blinks briefly at power-on.
2. Plug the ESP32 USB back in.
3. Open the serial monitor:
   ```
   pio device monitor
   ```
   (Exit with `Ctrl+C`.) If you have several COM ports, add `-p COM5`
   (use your port).
4. Press the ESP32's **EN / RST** button once so you see the boot output
   from the start.

The firmware copes with other power-up orders: you can press `r` later.
The order above is just the one that gives the cleanest output.

### 6. Check the startup output
You should see:

```
==== HerkulexTest firmware v2 ====
UART1  TX=GPIO17  RX=GPIO16  115200 baud
HerkulexTest: looking for the servo...
HerkulexTest: FOUND servo -- two-way communication OK
  servo ID <n>: no errors, torque ON      (<n> = your servo's ID; write it down)
HerkulexTest: READY
```

* **No `==== HerkulexTest firmware v2 ====` line at all?** The wrong
  firmware is on the board. Go back to step 2.
* **`no reply from any servo` / `BLIND mode`?** See Troubleshooting. You
  can still try spinning it.

### 7. Spin it
Click inside the monitor window first, then use these keys:

| Key | Action |
|---|---|
| **hold `d`** | spin clockwise (servo LED blue) |
| **hold `a`** | spin counter-clockwise (servo LED red) |
| let go | stops (within about ¼ s) |
| `s` | print the servo's status / errors |
| `r` | search for the servo again (use after changing power or wiring) |
| `h` | show the controls |

A single tap spins the servo for about 1 second. That's expected: the
firmware waits to see whether you're holding the key.

### 8. Power down
Unplug USB, then switch the supply output off. Then unplug the wires.

---

## Troubleshooting

Find the message you're seeing in the console:

| You see | Likely cause | Fix |
|---|---|---|
| Upload: `Bad data checksum`, or the upload drops out partway | Interference from the laptop's Bluetooth, or the wrong COM port | Turn **Bluetooth off**, unplug/replug the ESP32, and name the port: `pio run -e HerkulexTest -t upload --upload-port COM5` (use your port). |
| `COM5` "not available" but it shows in Device Manager | Another program already has the port open | Close any other serial monitor (another terminal, VS Code's monitor button, Arduino IDE, PuTTY). |
| Upload: `Failed to connect to ESP32` | Servo still wired to the ESP32, or a charge-only USB cable | Step 2. Try another cable. Hold BOOT during upload. |
| No `HerkulexTest firmware v2` banner | Wrong firmware on the board, or the monitor opened after boot | Press EN/RST. If the banner still doesn't appear, re-flash (step 3). |
| `no reply from any servo` and **`a`/`d` do nothing** | Servo has no power, or the ESP32 TX → servo RX wire or GND is wrong | Supply on? Servo LED lit at power-on? Check the GND wire, then the GPIO17 → servo RX wire. Press `r`. |
| `no reply from any servo` but **`a`/`d` DO spin it** (BLIND mode) | Commands reach the servo but its replies don't reach the ESP32 | Check the servo TX → GPIO16 wire. Spinning still works in this mode. |
| `LOST the servo` | Supply switched off or current-limited, or a wire came loose | Check the supply display and the wires, then press `r`. |
| `INPUT VOLTAGE OUT OF RANGE` | Supply voltage is wrong, or the current limit is too low so the voltage sags while spinning | Check the voltage. Raise the current limit a little. |
| `OVERLOAD` | Shaft is blocked or the servo is stalled | Free the shaft. The error clears automatically on the next spin. |
| `OVERHEATED` | Servo is too hot | Power off and let it cool. |
| `BAD PACKET received` | Electrical noise or a missing ground | Check the GND wire. Keep the jumper wires short. |
| Holding a key makes it spin, stop, spin | Windows keyboard repeat delay is set very long | Settings → Bluetooth & devices → Keyboard → set *Repeat delay* to short. |
| Typing does nothing at all | Monitor window not focused, or the monitor isn't connected | Click inside the terminal. Restart `pio device monitor`. |

Still stuck? Copy the **whole** console output, starting from the
`====` banner, and send it to the firmware team.

---

## For firmware developers

* Sources: `main.cpp` (test app), plus `src/BenchShared/` (servo helpers
  shared with `env:DBoxMotor`, and `ComsStub.cpp`, which stubs out the
  Pi/queue layer). They're built with the real `src/Herkulex` driver for all
  motion commands. `scripts/select_sources.py` picks the right sources for
  every PlatformIO environment and forces a CMake re-configure when a build
  dir was set up for a different source set.
* STAT and clear-error use local raw packets because the driver's
  `HerkulexClass::stat()` (checksum2 compare never matches) and
  `clearError()` (packet size 11 with only 10 bytes filled) have bugs. See
  the comment in `src/BenchShared/ServoBench.h`.
* The firmware re-sends clear-errors + torque-on before every spin, so the
  power-on order between the ESP32 and the servo doesn't matter.
* Next step up: the same servo driven from a laptop over D-BOX
  (`env:DBoxMotor` + `Crab_API/src/bench_test.py`). See
  `Crab_API/BENCH_TEST.md`.
