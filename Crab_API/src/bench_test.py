"""
Urchin bench test: check each link of the robot's motor path from a laptop,
using Gabe's D-BOX protocol, without needing to know anything about it.

    laptop --D-BOX over USB--> ESP32 --Herkulex over UART1--> servo

Menu options:
    1) Check ESP32 link   laptop <-> ESP32 only (works with DBoxEcho or DBoxMotor firmware)
    2) Check motor        ESP32 <-> servo: finds the servo, reports its status in plain English
    3) Spin motor         keyboard control, end to end
    4) Move to an angle   end to end

Needs:  pip install pyserial
Run from the Crab_API folder:
    python src/bench_test.py          (finds the ESP32 by itself)
    python src/bench_test.py COM5     (or name the port)

Step-by-step guide (flashing, wiring, troubleshooting): Crab_API/BENCH_TEST.md
ESP32 side: Urchin_Firmware/src/DBoxMotor/main.cpp (wire protocol documented there).
"""

import os
import random
import string
import sys
import time
from typing import Optional

import serial
import serial.tools.list_ports

from dbox import DBoxLink, Packet

IDENTITY = "Urchin"   # handshake string, see DBoxMotor/main.cpp
VPID = 0
SPIN_SPEED = 200      # same as HerkulexTest; servo range is -1023..1023
KEEPALIVE_S = 0.1     # firmware stops the motor if no Spin arrives for 0.5s
LINK_ROUNDS = 20
MOVE_TIME_MS = 1000
MAX_ANGLE = 160       # MODEL_0601 tops out around +/-164 degrees

ERROR_BITS = [
    (0x01, "INPUT VOLTAGE OUT OF RANGE - check the DC supply voltage / current limit"),
    (0x02, "POSITION LIMIT exceeded"),
    (0x04, "OVERHEATED - power off and let it cool"),
    (0x08, "BAD PACKET received - check the GND wire and keep jumper wires short"),
    (0x10, "OVERLOAD - shaft blocked or stalled"),
    (0x40, "EEPROM corrupted - needs Herkulex Manager"),
]

MOTOR_FW = "DBoxMotor"
ECHO_FW = "DBoxEcho"


def say(text: str = "") -> None:
    print(text, flush=True)


def hint(*lines: str) -> None:
    for line in lines:
        say("   -> " + line)


# --- Keyboard (single keypresses, no Enter) ---------------------------------

class Keys:
    """Non-blocking single-key reader for Windows and Linux/Pi terminals."""

    def __enter__(self) -> "Keys":
        if os.name == "nt":
            import msvcrt
            self._msvcrt = msvcrt
        else:
            import termios
            import tty
            self._termios = termios
            self._fd = sys.stdin.fileno()
            self._saved = termios.tcgetattr(self._fd)
            tty.setcbreak(self._fd)
        return self

    def __exit__(self, *exc) -> None:
        if os.name != "nt":
            self._termios.tcsetattr(self._fd, self._termios.TCSADRAIN, self._saved)

    def get(self) -> Optional[str]:
        if os.name == "nt":
            if self._msvcrt.kbhit():
                return self._msvcrt.getwch().lower()
            return None
        import select
        if select.select([sys.stdin], [], [], 0)[0]:
            return sys.stdin.read(1).lower()
        return None


# --- Connection ------------------------------------------------------------

class Bench:
    def __init__(self, link: DBoxLink, port: str, firmware: str, info: str) -> None:
        self.link = link
        self.port = port
        self.firmware = firmware
        self.info = info

    def request(self, text: str, timeout: float = 1.0) -> Optional[Packet]:
        """Send one command, return the reply (None if nothing came back)."""
        self.link.send(VPID, 'N', text)
        deadline = time.monotonic() + timeout
        while True:
            packet = self.link.receive(max(0.0, deadline - time.monotonic()))
            if packet is None:
                return None
            if packet.stream == 'D':
                say(f"   (ESP32 says: {packet.text})")
                continue
            if packet.stream == 'S':
                continue
            return packet


def try_port(port: str) -> Optional[Bench]:
    try:
        link = DBoxLink(port)
    except serial.SerialException as exc:
        if "PermissionError" in str(exc) or "Access is denied" in str(exc):
            say(f"   {port}: in use by another program (close any serial monitor and try again)")
        return None

    time.sleep(0.5)          # let the USB-serial chip / ESP32 settle (it may reboot on open)
    link.discard_input()     # drop boot messages

    for _ in range(4):
        link.send(0, 'S', IDENTITY)
        deadline = time.monotonic() + 1.0
        while True:
            packet = link.receive(max(0.0, deadline - time.monotonic()))
            if packet is None:
                break
            if packet.vpid == 0 and packet.stream == 'S' and packet.text == IDENTITY:
                return identify(link, port)
    link.close()
    return None


def identify(link: DBoxLink, port: str) -> Optional[Bench]:
    bench = Bench(link, port, "", "")
    reply = bench.request("Info")
    if reply is not None and reply.stream == 'E' and reply.text.startswith("OK " + MOTOR_FW):
        bench.firmware = MOTOR_FW
        bench.info = reply.text[3:]
    elif reply is not None and reply.stream == 'N' and reply.text == "Info":
        bench.firmware = ECHO_FW   # echo firmware just sends our packet back
        bench.info = ECHO_FW
    else:
        bench.firmware = "unknown"
        bench.info = "unrecognised firmware"
    return bench


def connect(port_arg: Optional[str]) -> Optional[Bench]:
    if port_arg:
        ports = [port_arg]
    else:
        ports = [p.device for p in serial.tools.list_ports.comports()
                 if "bluetooth" not in (p.description or "").lower()]
    if not ports:
        say("No serial ports found.")
        hint("Plug the ESP32 in with a DATA USB cable (charge-only cables won't work).")
        return None

    say(f"Looking for the ESP32 on: {', '.join(ports)}")
    for port in ports:
        bench = try_port(port)
        if bench is not None:
            return bench

    say("FAIL: no ESP32 answered the D-BOX handshake.")
    hint("Is the ESP32 plugged in, and flashed with DBoxMotor (or DBoxEcho) firmware?",
         "   flash it from Urchin_Firmware with:  pio run -e DBoxMotor -t upload",
         "Close any serial monitor (pio device monitor, VS Code, Arduino IDE) -- only one",
         "   program can use the port at a time.",
         "Press the ESP32's EN/RST button and run this again.")
    return None


# --- Tests -----------------------------------------------------------------

def check_link(bench: Bench) -> None:
    say(f"\n[1] ESP32 link: {LINK_ROUNDS} round trips over D-BOX...")
    passed = 0
    times = []
    for i in range(LINK_ROUNDS):
        start = time.monotonic()
        if bench.firmware == ECHO_FW:
            payload = "".join(random.choices(string.ascii_letters + string.digits, k=random.randint(1, 200)))
            reply = bench.request(payload)
            ok = reply is not None and reply.stream == 'N' and reply.text == payload
        else:
            reply = bench.request("Info")
            ok = reply is not None and reply.text.startswith("OK " + MOTOR_FW)
        if ok:
            passed += 1
            times.append((time.monotonic() - start) * 1000)
    if passed == LINK_ROUNDS:
        say(f"PASS: {passed}/{LINK_ROUNDS} round trips OK, average {sum(times) / len(times):.0f} ms")
    else:
        say(f"FAIL: only {passed}/{LINK_ROUNDS} round trips came back correctly")
        hint("Try a different USB cable / port, plugged straight into the laptop (no hub).",
             "Turn Bluetooth off (it has interfered with this USB link before).")


def describe_status(reply_text: str) -> None:
    # "OK STAT <id> <error> <detail>"
    _, _, servo_id, error, detail = reply_text.split()
    error, detail = int(error), int(detail)
    say(f"   servo ID {servo_id}, torque {'ON' if detail & 0x40 else 'OFF'}")
    if error == 0:
        say("   no errors")
    for bit, text in ERROR_BITS:
        if error & bit:
            say(f"   ERROR: {text}")
    if error:
        hint("Errors are cleared automatically on the next Spin/Move.")


def check_motor(bench: Bench) -> None:
    say("\n[2] Motor: looking for the servo (can take up to ~10 seconds)...")
    reply = bench.request("Find", timeout=20)
    if reply is None:
        say("FAIL: the ESP32 stopped answering.")
        hint("Check the USB cable; press EN/RST on the ESP32 and run this again.")
        return
    if reply.text.startswith("OK FOUND"):
        say(f"PASS: servo found (ID {reply.text.split()[2]}) -- two-way communication OK")
        status = bench.request("Status")
        if status is not None and status.text.startswith("OK STAT"):
            describe_status(status.text)
        return
    say("FAIL: no servo answered.")
    hint("Is the DC supply ON? Did the servo's LED light up at power-on?",
         "Check the GND wire between the servo and the ESP32.",
         "Check servo TX -> ESP32 GPIO16 (the servo's replies travel on this wire).",
         "You can still try option 3: if the motor spins anyway, only that",
         "   servo TX -> GPIO16 wire is the problem.")


def spin_motor(bench: Bench) -> None:
    say("\n[3] Spin motor:  d = spin clockwise   a = spin counter-clockwise")
    say("                 space = stop         q = stop and go back to the menu")
    direction = 0
    last_sent = 0.0
    with Keys() as keys:
        while True:
            key = keys.get()
            if key == 'q':
                break
            new_direction = {'d': 1, 'a': -1, ' ': 0, 's': 0}.get(key, direction) if key else direction
            if new_direction != direction:
                direction = new_direction
                if direction == 0:
                    reply = bench.request("Stop")
                    say("   stopped" if reply is not None else "   (no reply to Stop -- check the USB link)")
                else:
                    say(f"   spinning {'clockwise' if direction > 0 else 'counter-clockwise'}"
                        " (servo LED " + ("blue" if direction > 0 else "red") + ")")
                last_sent = 0.0
            now = time.monotonic()
            if direction != 0 and now - last_sent >= KEEPALIVE_S:
                reply = bench.request(f"Spin {direction * SPIN_SPEED}", timeout=0.5)
                if reply is None:
                    say("   FAIL: the ESP32 stopped answering -- motor will stop on its own")
                    direction = 0
                last_sent = now
            time.sleep(0.01)
    bench.request("Stop")
    say("   stopped")


def move_motor(bench: Bench) -> None:
    raw = input(f"\n[4] Angle to move to, in degrees (-{MAX_ANGLE} to {MAX_ANGLE}): ").strip()
    try:
        angle = float(raw)
    except ValueError:
        say("   That's not a number.")
        return
    if abs(angle) > MAX_ANGLE:
        say(f"   Please pick an angle between -{MAX_ANGLE} and {MAX_ANGLE}.")
        return
    reply = bench.request(f"Move {angle} {MOVE_TIME_MS}")
    if reply is None:
        say("FAIL: no reply from the ESP32.")
    elif reply.text.startswith("OK MOVE"):
        say(f"   moving to {angle:g} degrees (servo LED green)")
        hint("If it didn't move: run option 2 to check the servo.")
    else:
        say(f"FAIL: ESP32 refused the move ({reply.text}).")


# --- Menu ------------------------------------------------------------------

def main() -> int:
    say("==== Urchin bench test ====")
    bench = connect(sys.argv[1] if len(sys.argv) > 1 else None)
    if bench is None:
        return 1

    say(f"Connected on {bench.port}: {bench.info}")
    motor_ok = bench.firmware == MOTOR_FW
    if not motor_ok:
        say(f"Note: motor options need DBoxMotor firmware (this board runs {bench.firmware}).")
        hint("Flash it from Urchin_Firmware with:  pio run -e DBoxMotor -t upload")

    needs = "" if motor_ok else "   (needs DBoxMotor firmware)"
    try:
        while True:
            say("\n 1) Check ESP32 link")
            say(" 2) Check motor" + needs)
            say(" 3) Spin motor" + needs)
            say(" 4) Move motor to an angle" + needs)
            say(" q) Quit")
            choice = input("Choose: ").strip().lower()
            if choice == '1':
                check_link(bench)
            elif choice in ('2', '3', '4') and not motor_ok:
                say("   This needs DBoxMotor firmware on the ESP32 (see the note above).")
            elif choice == '2':
                check_motor(bench)
            elif choice == '3':
                spin_motor(bench)
            elif choice == '4':
                move_motor(bench)
            elif choice in ('q', 'quit', 'exit'):
                break
    except serial.SerialException:
        say("\nFAIL: lost the connection to the ESP32 (USB unplugged?). The motor stops on its own.")
        return 1
    except (KeyboardInterrupt, EOFError):
        say("")
    finally:
        try:
            if motor_ok:
                bench.request("Stop", timeout=0.3)
            bench.link.close()
        except serial.SerialException:
            pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
