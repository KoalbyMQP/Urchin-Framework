# Urchin Hardware Bench Tests: Quick Reference

These are standalone test programs for checking the motor path on real
hardware, one link at a time. They are **not** the normal Urchin firmware.
Each one builds only the code it tests, using the team's real drivers.

```
  laptop  ──USB (D-BOX)──►  ESP32  ──UART (Herkulex)──►  servo
  └──────── DBoxEcho ──────┘       └──── HerkulexTest ────┘
  └──────────────────── DBoxMotor (end to end) ──────────────┘
```

| Test | What it proves | Full guide |
|---|---|---|
| `DBoxEcho` | Laptop ↔ ESP32 over D-BOX (no servo) | this page |
| `HerkulexTest` | ESP32 ↔ servo, keyboard control from a serial monitor | [`Urchin_Firmware/src/HerkulexTest/README.md`](Urchin_Firmware/src/HerkulexTest/README.md) |
| `DBoxMotor` | Laptop → ESP32 → servo, menu-driven PASS/FAIL | [`Crab_API/BENCH_TEST.md`](Crab_API/BENCH_TEST.md) |

Wiring, power settings and troubleshooting are in the full guides. This page
is just the commands.

---

## One-time setup

```
git fetch
git checkout UV-Test
git pull
```

* VS Code with the **PlatformIO** extension (gives you the `pio` command).
* Python 3.11+ and pyserial: from `Crab_API/`, run `pip install pyserial`.

---

## Rules that apply to every test

1. **Flash with the servo unplugged from the ESP32.** If the servo wiring is
   connected, uploads fail with `Failed to connect to ESP32`.
2. **Turn Bluetooth off before flashing.** It caused upload errors on our
   test laptop.
3. **Only one program can use the COM port at a time.** Close
   `pio device monitor` before running `bench_test.py`, and the other way
   round.
4. Wiring: ESP32 **GPIO17 (TX) → servo RX**, **GPIO16 (RX) ← servo TX**,
   **shared GND**. The servo is powered **only** by the DC supply, never by
   the ESP32.
5. You do **not** need to set any environment variables. The build picks
   the right code automatically (`Urchin_Firmware/scripts/select_sources.py`).

If you have more than one serial device, add the port to any command:
`--upload-port COM5` for uploads, `-p COM5` for the monitor, or `COM5` as an
argument to `bench_test.py`.

---

## 1. DBoxEcho: laptop ↔ ESP32 link only

```
cd Urchin_Firmware
pio run -e DBoxEcho -t upload          # flash (USB only)

cd ../Crab_API
python src/bench_test.py               # then choose option 1
```

PASS: `PASS: 20/20 round trips OK`. The motor options won't work with this
firmware; that's expected.

---

## 2. HerkulexTest: ESP32 ↔ servo only

```
cd Urchin_Firmware
pio run -e HerkulexTest -t upload      # flash (servo unplugged)
```

Unplug USB → connect the servo wires → DC supply on → plug USB back in:

```
pio device monitor                     # Ctrl+C to exit
```

Press **EN/RST** on the ESP32. Expect `==== HerkulexTest firmware v2 ====`
followed by `FOUND servo`. Then, with the monitor window focused:

| Key | Action |
|---|---|
| hold `d` / hold `a` | spin clockwise / counter-clockwise |
| `s` | servo status / errors |
| `r` | search for the servo again |
| `h` | help |

---

## 3. DBoxMotor: laptop → ESP32 → servo (end to end)

```
cd Urchin_Firmware
pio run -e DBoxMotor -t upload         # flash (servo unplugged)
```

Unplug USB → connect the servo wires → DC supply on → plug USB back in:

```
cd ../Crab_API
python src/bench_test.py
```

| Menu option | Checks |
|---|---|
| 1) Check ESP32 link | laptop ↔ ESP32 |
| 2) Check motor | ESP32 ↔ servo, with readable error messages |
| 3) Spin motor | `d` / `a` spin, space stops, `q` goes back |
| 4) Move motor to an angle | moves to a set angle |

The motor stops by itself if the program closes or the USB cable is pulled.

---

## Back to the normal robot firmware

```
cd Urchin_Firmware
pio run -e Urchin-Firmware -t upload
```

---

## Status / known issues

* The servo is addressed with the broadcast ID (`0xFE`). The real ID is
  printed at startup if the servo replies.
* The original driver's `HerkulexClass::stat()` and `clearError()` have
  bugs (details in `Urchin_Firmware/src/BenchShared/ServoBench.h`). The
  bench tests use their own versions; **the driver itself is unchanged**.
* For developers, the Python D-BOX implementation is checked against C++
  golden frames in `Crab_API/test/test_dbox.py`.

Problems? Copy the **whole** console output and send it to the firmware team.
