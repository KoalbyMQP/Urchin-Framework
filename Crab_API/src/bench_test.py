"""
Urchin bench test: check each link of the robot's motor path from a laptop
or the Raspberry Pi, using Gabe's D-BOX protocol, without needing to know
anything about it.

    laptop/Pi --D-BOX over USB--> ESP32 --Herkulex over UART1--> servo

Menu options:
    1) Check ESP32 link   laptop <-> ESP32 only (works with DBoxEcho or DBoxMotor firmware)
    2) Check motors       ESP32 <-> servos: finds every servo on the bus, reports
                          each one's status in plain English
    3) Spin motor         keyboard control, end to end
    4) Move to an angle   end to end
    5) Choose motor       which servo options 3/4 drive: one of them, or all at once
    6) Change motor ID    give a servo a new Herkulex ID (needed before two servos
                          with the same ID can share the bus)
    7) Speed scan         finds a servo that doesn't answer because it was set to
                          another baud rate, and sets it back to 115200
    8) Synchronized move  several motors, each to its own angle, all starting on
                          the same instant (one Herkulex packet: S_JOG or I_JOG)

Several servos can share the bus (daisy chain); each needs its own ID. Each
servo is addressed by D-BOX VPID = its Herkulex ID, and VPID 0 means "all of
them" (provisional, see DBoxMotor/main.cpp). Works with one servo too.
Servos can also be split over two buses (bus 2 = GPIO26/27, for servo models
that can't share a wire); option 2 shows each servo's bus.

D-BOX comes from DBOXpy, the Python bindings of the team's C++ D-BOX code
(Clam/D-BOX): the same framing, receive and discovery code that runs on the
Pi. Linux only: run it on the Raspberry Pi, or on Windows through WSL with the
ESP32 attached via usbipd. Building DBOXpy: Clam/D-BOX/tools/README.md.

Run from the Crab_API folder:
    sudo env PYTHONPATH=$HOME/dbox python3 src/bench_test.py                (finds the ESP32 by itself)
    sudo env PYTHONPATH=$HOME/dbox python3 src/bench_test.py /dev/ttyUSB0   (or name the port)

Step-by-step guide (flashing, wiring, troubleshooting): Crab_API/BENCH_TEST.md
ESP32 side: Urchin_Firmware/src/DBoxMotor/main.cpp (wire protocol documented there).
"""

import os
import random
import string
import sys
import time
from typing import Optional

try:
    import DBOXpy as dbox
except ImportError:
    print("Can't find the DBOXpy module (the team's D-BOX code for Python).\n"
          "   Build it first: see Clam/D-BOX/tools/README.md, 'From Python'.\n"
          "   Then run with PYTHONPATH pointing at the folder that holds DBOXpy*.so, e.g.\n"
          "   sudo env PYTHONPATH=$HOME/dbox python3 src/bench_test.py", flush=True)
    sys.exit(1)

IDENTITY = "Urchin"   # handshake string, see DBoxMotor/main.cpp
ALL = 0               # VPID 0 = the board / every servo at once
SPIN_SPEED = 200      # same as HerkulexTest; servo range is -1023..1023
KEEPALIVE_S = 0.1     # firmware stops the motor if no Spin arrives for 0.5s
LINK_ROUNDS = 20
MOVE_TIME_MS = 1000
MAX_ANGLE = 160       # DRS-0601 tops out around +/-164 degrees, DRS-0201 +/-166

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

BAUD = 115200
READ_TIMEOUT_MS = 20       # how long one low-level read waits before giving up
HANDSHAKE_TIMEOUT_MS = 1000
HANDSHAKE_ATTEMPTS = 4


class Link:
    """One D-BOX connection, through DBOXpy (Gabe's C++ D-BOX)."""

    def __init__(self) -> None:
        self.config = dbox.DconfigUart(IDENTITY, "", BAUD, READ_TIMEOUT_MS)
        self.discovery = dbox.UartDiscovery(True, self.config)
        self.discovery.handshake_timeout_ms = HANDSHAKE_TIMEOUT_MS
        self.port = dbox.LinuxSerialPort(self.discovery)
        self.proto = dbox.DBOXprotocol(self.port, "/dev/null")

    def find_ports(self) -> list:
        return list(self.port.find())

    def handshake(self, port: str) -> bool:
        """D-BOX discovery on one port: say IDENTITY, wait for it back."""
        return self.discovery.active_discover_single(port, HANDSHAKE_ATTEMPTS)

    def open(self, port: str) -> bool:
        return self.port.openraw(port, BAUD, READ_TIMEOUT_MS)

    def send(self, vpid: int, stream: str, text: str) -> None:
        self.proto.send(dbox.DBOX(vpid, stream, text))

    def receive(self, timeout: float) -> Optional["dbox.DBOX"]:
        """Next valid packet, or None if none arrives within timeout seconds."""
        packet = dbox.DBOX()
        if self.proto.receive(packet, max(0, int(timeout * 1000))):
            return packet
        return None

    def close(self) -> None:
        self.port.close()


class Bench:
    def __init__(self, link: Link, port: str, firmware: str, info: str) -> None:
        self.link = link
        self.port = port
        self.firmware = firmware
        self.info = info
        self.servos = {}        # Herkulex ID -> model ("0601", "0201", ...)
        self.target = ALL       # VPID options 3/4 send to
        self.multi = False      # firmware understands per-servo VPIDs (v2+)

    def request(self, text: str, timeout: float = 1.0,
                vpid: Optional[int] = None) -> Optional["dbox.DBOX"]:
        """Send one command, return the reply (None if nothing came back)."""
        vpid = self.target if vpid is None else vpid
        self.link.send(vpid, 'N', text)
        deadline = time.monotonic() + timeout
        while True:
            packet = self.link.receive(max(0.0, deadline - time.monotonic()))
            if packet is None:
                return None
            if packet.stream == 'D':
                who = f"motor {packet.VPID}" if packet.VPID else "ESP32"
                say(f"   ({who} says: {packet.text})")
                continue
            if packet.stream == 'S':
                continue
            return packet

    def update_servos(self, info: str) -> None:
        """Read the servo list out of an Info reply.

        v2: "DBoxMotor v2 servos=219:0601,253:0201" (or servos=none)
        v1: "DBoxMotor v1 servo=219" (or servo=broadcast) -- one servo only
        """
        self.servos = {}
        for word in info.split():
            if word.startswith("servos=") and word != "servos=none":
                self.multi = True
                self.servos = parse_servo_list(word[len("servos="):].replace(",", " "))
            elif word == "servos=none":
                self.multi = True
            elif word.startswith("servo=") and word[6:].isdigit():
                self.servos = {int(word[6:]): "0601"}
        if self.target != ALL and self.target not in self.servos:
            self.target = ALL

    def target_name(self) -> str:
        if self.target == ALL:
            if len(self.servos) > 1:
                return f"all {len(self.servos)} motors"
            return "the motor" if self.servos else "the motor (not found yet)"
        return motor_name(self.target, self.servos.get(self.target, "?"))


def parse_servo_list(text: str) -> dict:
    """"219:0601 253:0201?" -> {219: "0601", 253: "0201?"} ('?' = model guessed)."""
    servos = {}
    for item in text.split():
        servo_id, _, model = item.partition(":")
        if servo_id.isdigit():
            servos[int(servo_id)] = model or "?"
    return servos


def motor_name(servo_id: int, model: str) -> str:
    # model as the firmware reports it: "0601", "0201?" (guessed), "0601@2" (bus 2)
    model, _, bus = model.partition("@")
    guessed = model.endswith("?")
    model = model.rstrip("?")
    text = f"motor {servo_id} (DRS-{model}" if model else f"motor {servo_id} ("
    if bus:
        text += f", bus {bus}"
    return text + (", model not confirmed)" if guessed else ")")


def try_port(link: Link, port: str) -> Optional[Bench]:
    if not os.access(port, os.R_OK | os.W_OK):
        say(f"   {port}: no permission (run with sudo, or: sudo usermod -aG dialout $USER and log in again)")
        return None
    if not link.handshake(port):
        return None
    if not link.open(port):
        say(f"   {port}: answered, but could not be reopened (in use by another program?)")
        return None
    return identify(link, port)


def identify(link: Link, port: str) -> Optional[Bench]:
    bench = Bench(link, port, "", "")
    reply = bench.request("Info")
    if reply is not None and reply.stream == 'E' and reply.text.startswith("OK " + MOTOR_FW):
        bench.firmware = MOTOR_FW
        bench.info = reply.text[3:]
        bench.update_servos(bench.info)
    elif reply is not None and reply.stream == 'N' and reply.text == "Info":
        bench.firmware = ECHO_FW   # echo firmware just sends our packet back
        bench.info = ECHO_FW
    else:
        bench.firmware = "unknown"
        bench.info = "unrecognised firmware"
    return bench


def connect(port_arg: Optional[str]) -> Optional[Bench]:
    link = Link()
    ports = [port_arg] if port_arg else link.find_ports()
    if not ports:
        say("No serial ports found.")
        hint("Plug the ESP32 in with a DATA USB cable (charge-only cables won't work).",
             "On WSL: attach it first, in PowerShell:  usbipd attach --wsl --busid <BUSID>",
             "   (usbipd list shows the BUSID of 'Silicon Labs CP210x').")
        return None

    say(f"Looking for the ESP32 on: {', '.join(ports)}")
    for port in ports:
        bench = try_port(link, port)
        if bench is not None:
            return bench

    say("FAIL: no ESP32 answered the D-BOX handshake.")
    hint("Is the ESP32 plugged in, and flashed with DBoxMotor (or DBoxEcho) firmware?",
         "   flash it from Urchin_Firmware with:  pio run -e DBoxMotor -t upload",
         "   (on WSL, give it back to Windows first:  usbipd detach --busid <BUSID>)",
         "Close any serial monitor (pio device monitor, VS Code, Arduino IDE) -- only one",
         "   program can use the port at a time.",
         "Press the ESP32's EN/RST button and run this again.",
         "Just updated the code? Rebuild DBOXpy AND re-flash the ESP32: both sides",
         "   must run the same D-BOX version (e.g. the message CRC-32 changed the",
         "   packet format), or they can't understand each other.")
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
            reply = bench.request("Info", vpid=ALL)
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


def describe_status(bench: Bench, reply_text: str) -> None:
    # "OK STAT <id> <error> <detail> [<id> <error> <detail> ...]", "-" = no reply
    words = reply_text.split()[2:]
    any_error = False
    for i in range(0, len(words) - 2, 3):
        servo_id, error, detail = words[i:i + 3]
        name = motor_name(int(servo_id), bench.servos.get(int(servo_id), "?"))
        if error == "-":
            say(f"   {name}: did not answer this time")
            continue
        error, detail = int(error), int(detail)
        say(f"   {name}: torque {'ON' if detail & 0x40 else 'OFF'}, "
            + ("no errors" if error == 0 else "errors:"))
        for bit, text in ERROR_BITS:
            if error & bit:
                say(f"      ERROR: {text}")
        any_error = any_error or error != 0
    if any_error:
        hint("Errors are cleared automatically on the next Spin/Move.")


def check_motor(bench: Bench) -> None:
    say("\n[2] Motors: looking for every servo on both buses (takes about 20 seconds)...")
    reply = bench.request("Find", timeout=35, vpid=ALL)
    if reply is None:
        say("FAIL: the ESP32 stopped answering.")
        hint("Check the USB cable; press EN/RST on the ESP32 and run this again.")
        return
    if reply.text.startswith("OK FOUND"):
        if bench.multi:
            bench.servos = parse_servo_list(reply.text[len("OK FOUND"):])
        else:
            bench.servos = {int(reply.text.split()[2]): "0601"}
        if bench.target != ALL and bench.target not in bench.servos:
            bench.target = ALL
        count = len(bench.servos)
        say(f"PASS: {count} servo{'s' if count != 1 else ''} found -- two-way communication OK")
        for servo_id, model in bench.servos.items():
            say(f"   {motor_name(servo_id, model)}")
        status = bench.request("Status", vpid=ALL)
        if status is not None and status.text.startswith("OK STAT"):
            describe_status(bench, status.text)
        if count > 1:
            hint("Options 3 and 4 drive all of them; use option 5 to pick one.")
        if any(model.endswith("?") for model in bench.servos.values()):
            hint("A model ending in '?' was guessed (the servo didn't say what it is).",
                 "   If angles come out wrong for it, tell the firmware team its model.")
        return
    say("FAIL: no servo answered.")
    hint("Is the DC supply ON? Did the servo's LED light up at power-on?",
         "Check the GND wire between the servo and the ESP32.",
         "Check servo TX -> ESP32 GPIO16 (the servo's replies travel on this wire).",
         "You can still try option 3: if the motor spins anyway, only that",
         "   servo TX -> GPIO16 wire is the problem.")


def spin_motor(bench: Bench) -> None:
    say(f"\n[3] Spin {bench.target_name()}:")
    say("                 d = spin clockwise   a = spin counter-clockwise")
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
                elif reply.text.startswith("ERR"):
                    say(f"   FAIL: ESP32 refused ({reply.text}) -- run option 2")
                    direction = 0
                last_sent = now
            time.sleep(0.01)
    bench.request("Stop")
    say("   stopped")


def move_motor(bench: Bench) -> None:
    raw = input(f"\n[4] Angle to move {bench.target_name()} to, in degrees"
                f" (-{MAX_ANGLE} to {MAX_ANGLE}): ").strip()
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


def choose_motor(bench: Bench) -> None:
    if not bench.servos:
        say("\n[5] No motors known yet -- run option 2 first.")
        return
    say("\n[5] Which motor should options 3 and 4 drive?")
    say("    0) all of them at once")
    for servo_id, model in bench.servos.items():
        say(f"  {servo_id:3}) {motor_name(servo_id, model)}")
    raw = input("Motor ID: ").strip()
    if not raw.isdigit() or (int(raw) != ALL and int(raw) not in bench.servos):
        say("   Not one of the IDs above; nothing changed.")
        return
    bench.target = int(raw)
    say(f"   Options 3 and 4 now drive {bench.target_name()}.")


def change_id(bench: Bench) -> None:
    say("\n[6] Change a motor's ID. Every servo on the bus needs its own ID;")
    say("    new servos all come with the same factory ID (DRS-0601: 219, DRS-0201: 253),")
    say("    so plug them in ONE AT A TIME and give each a new ID before adding the next.")
    if not bench.servos:
        say("   No motors known yet -- run option 2 first.")
        return
    for servo_id, model in bench.servos.items():
        say(f"  {servo_id:3}) {motor_name(servo_id, model)}")
    old = input("Current ID of the motor to change: ").strip()
    if not old.isdigit() or int(old) not in bench.servos:
        say("   Not one of the IDs above; nothing changed.")
        return
    new = input("New ID (1-253): ").strip()
    if not new.isdigit() or not 1 <= int(new) <= 253:
        say("   The new ID must be a number from 1 to 253; nothing changed.")
        return
    old, new = int(old), int(new)
    reply = bench.request(f"SetId {new}", timeout=5, vpid=old)
    if reply is None:
        say("FAIL: no reply from the ESP32.")
    elif reply.text.startswith("OK SETID"):
        bench.servos[new] = bench.servos.pop(old)
        bench.servos = dict(sorted(bench.servos.items()))
        if bench.target == old:
            bench.target = new
        say(f"PASS: motor {old} is now motor {new} (saved in the servo, survives power-off)")
        hint("Write the new ID on a piece of tape on the servo.")
    elif reply.text.startswith("ERR TAKEN"):
        say(f"FAIL: another servo already uses ID {new}. Pick a different one.")
    else:
        say(f"FAIL: the servo didn't take the new ID ({reply.text}).")
        hint("Run option 2 to see which IDs answer now.")


def baud_scan(bench: Bench) -> None:
    say("\n[7] Speed scan: finds a servo set to a different baud rate (speed).")
    say("    Connect ONLY the servo you're checking, power it at the right voltage,")
    say("    and make sure its LED is not blinking red.")
    if input("Only that one servo connected? (y/n): ").strip().lower() != 'y':
        say("   Nothing done.")
        return
    say("   Trying every Herkulex speed (a few seconds)...")
    reply = bench.request("BaudScan", timeout=10, vpid=ALL)
    if reply is None:
        say("FAIL: no reply from the ESP32.")
        return
    words = reply.text.split()
    if reply.text == "OK BAUD NONE":
        say("FAIL: the servo didn't answer at any speed.")
        hint("So it's not a speed problem. Check: supply voltage (DRS-0201: 7-10 V),",
             "   GND shared with the ESP32, and TXD/RXD crossed to the ESP32's RX/TX.")
        return
    if len(words) not in (4, 5) or not words[2].isdigit():
        say(f"FAIL: unexpected reply ({reply.text}) -- re-flash DBoxMotor.")
        return
    servo_id, baud = int(words[2]), int(words[3])
    bus = int(words[4]) if len(words) == 5 else 1
    if baud == BAUD:
        say(f"PASS: motor {servo_id} answers at {BAUD} already -- speed is fine.")
        hint("Run option 2 to add it.")
        return
    say(f"FOUND: motor {servo_id} is set to {baud} baud (the bench uses {BAUD}).")
    if input(f"Set motor {servo_id} to {BAUD} so it works with the bench? (y/n): ").strip().lower() != 'y':
        say("   Nothing changed.")
        return
    reply = bench.request(f"FixBaud {servo_id} {baud} {bus}", timeout=5, vpid=ALL)
    if reply is not None and reply.text.startswith("OK FIXBAUD"):
        say(f"PASS: motor {servo_id} now uses {BAUD} (saved in the servo).")
        hint("Run option 2: it should show up now.")
    else:
        say(f"FAIL: the servo didn't switch ({reply.text if reply else 'no reply'}).")


def sync_move(bench: Bench) -> None:
    say("\n[8] Synchronized move: give each motor its own angle; they all start")
    say("    on the same instant. Leave an angle blank to keep that motor still.")
    if not bench.servos:
        say("   No motors known yet -- run option 2 first.")
        return
    reply = bench.request("Clear", vpid=ALL)
    if reply is None or not reply.text.startswith("OK"):
        say("FAIL: this firmware has no synchronized moves -- re-flash DBoxMotor.")
        return
    queued, own_times, need_shared = 0, False, False
    for servo_id, model in bench.servos.items():
        raw = input(f"  {motor_name(servo_id, model)}: angle (-{MAX_ANGLE} to {MAX_ANGLE}, "
                    "blank = stay): ").strip()
        if not raw:
            continue
        try:
            angle = float(raw)
        except ValueError:
            say("   Not a number; skipping this motor.")
            continue
        own = input("    its own travel time in ms (blank = same as the others): ").strip()
        if own and not own.isdigit():
            say("   Time must be a whole number; using the shared time.")
            own = ""
        own_times = own_times or bool(own)
        need_shared = need_shared or not own
        reply = bench.request(f"Queue {angle}" + (f" {own}" if own else ""), vpid=servo_id)
        if reply is None or not reply.text.startswith("OK QUEUED"):
            say(f"   FAIL: motor {servo_id} refused it ({reply.text if reply else 'no reply'}).")
            bench.request("Clear", vpid=ALL)
            return
        queued += 1
    if queued == 0:
        say("   Nothing queued; no motor moves.")
        return
    go = "Go"  # every motor has its own time: no shared time to ask for
    if need_shared:
        shared = input("Shared travel time in ms [1000]: ").strip() or "1000"
        go = f"Go {shared if shared.isdigit() else '1000'}"
    reply = bench.request(go, vpid=ALL, timeout=2)
    if reply is not None and reply.text.startswith("OK GO"):
        _, _, count, kind = reply.text.split()
        how = ("each arriving on its own time" if kind == "IJOG"
               else "all arriving together")
        say(f"PASS: {count} motor{'s' if count != '1' else ''} started on the same instant, "
            f"{how} ({kind} packet)")
    else:
        say(f"FAIL: {reply.text if reply else 'no reply from the ESP32'}")


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
    multi_needs = needs or ("" if bench.multi else "   (needs DBoxMotor v2 firmware)")
    try:
        while True:
            if motor_ok:
                say(f"\n Driving: {bench.target_name()}")
            say("\n 1) Check ESP32 link")
            say(" 2) Check motors" + needs)
            say(" 3) Spin motor" + needs)
            say(" 4) Move motor to an angle" + needs)
            say(" 5) Choose motor" + multi_needs)
            say(" 6) Change a motor's ID" + multi_needs)
            say(" 7) Speed scan (servo not found?)" + multi_needs)
            say(" 8) Synchronized move" + multi_needs)
            say(" q) Quit")
            choice = input("Choose: ").strip().lower()
            if choice == '1':
                check_link(bench)
            elif choice in ('2', '3', '4', '5', '6', '7', '8') and not motor_ok:
                say("   This needs DBoxMotor firmware on the ESP32 (see the note above).")
            elif choice in ('5', '6', '7', '8') and not bench.multi:
                say("   This needs the multi-motor (v2) DBoxMotor firmware; re-flash it.")
            elif choice == '2':
                check_motor(bench)
            elif choice == '3':
                spin_motor(bench)
            elif choice == '4':
                move_motor(bench)
            elif choice == '5':
                choose_motor(bench)
            elif choice == '6':
                change_id(bench)
            elif choice == '7':
                baud_scan(bench)
            elif choice == '8':
                sync_move(bench)
            elif choice in ('q', 'quit', 'exit'):
                break
    except (KeyboardInterrupt, EOFError):
        say("")
    finally:
        if motor_ok:
            bench.request("Stop", timeout=0.3, vpid=ALL)
        bench.link.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
