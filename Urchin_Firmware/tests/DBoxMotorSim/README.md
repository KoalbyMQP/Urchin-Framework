# DBoxMotor simulator (no ESP32, no servos needed)

Runs the **real** `src/DBoxMotor` firmware on Linux (WSL or the Pi) with a
pretend board around it:

```
bench_test.py ──D-BOX──► /dev/pts/N ──► DBoxMotor firmware ──► simulated Herkulex bus
   (unchanged)          (pretend USB)     (unchanged code)      (DRS-0601, DRS-0201, ...)
```

Use it to try multi-motor features before you own the motors, or to check a
firmware change before flashing. Only `sim.cpp` and the stand-in ESP-IDF /
FreeRTOS headers in `include/` are fake; everything they wrap is the code
that gets flashed.

## Run it

```
bash Urchin_Firmware/tests/DBoxMotorSim/build.sh
SIM_SERVOS="219:0601,253:0201" SIM_LOG=servos.log Urchin_Firmware/tests/DBoxMotorSim/dboxmotor_sim
```

It prints the pretend port (e.g. `/dev/pts/3`) and keeps running. In a second
terminal, from `Crab_API`:

```
env PYTHONPATH=$HOME/dbox python3 src/bench_test.py /dev/pts/3
```

`servos.log` shows what each simulated servo was told to do
(`SERVO 253 (DRS-0201) MOVE to position 650`, `SPIN 200`, `STOP`, ...).

* `SIM_SERVOS`: comma-separated `<id>:<model>`; model is 0101, 0201, 0601 or
  0602. Two servos with the same ID collide, like on the real bus. Empty =
  no servo powered.
* The simulated servos check every packet's checksum, answer STAT and EEPROM
  reads, take EEPROM ID writes and reboots, and collide when more than one
  answers a broadcast.

Not simulated: real motion and timing, voltage/overload errors, and the
DRS-0601/0602 model numbers (not yet confirmed on hardware).
