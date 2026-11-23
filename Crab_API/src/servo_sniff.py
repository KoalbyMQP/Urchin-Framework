"""
Servo reply sniffer -- for a Herkulex servo that obeys commands but is never
found by bench_test.py option 2.

It asks the DBoxMotor firmware to listen to the servo reply wire (servo TXD ->
ESP32 GPIO16) and report every raw byte, including garbled ones that the
normal servo check throws away. Then it says in plain English what that means.

Run from Crab_API (needs DBoxMotor firmware with the Sniff command, and
DBOXpy built as in Clam/D-BOX/tools/README.md):

    sudo env PYTHONPATH=$HOME/dbox python3 src/servo_sniff.py
    sudo env PYTHONPATH=$HOME/dbox python3 src/servo_sniff.py --id 253
    sudo env PYTHONPATH=$HOME/dbox python3 src/servo_sniff.py --bus 2   (servos on GPIO26/27)
    sudo env PYTHONPATH=$HOME/dbox python3 src/servo_sniff.py /dev/ttyUSB1

Connect ONLY the servo you are checking: by default it pings the broadcast
ID, and two servos answering at once would garble each other.
"""

import sys
from typing import List, Optional, Tuple

from bench_test import ALL, connect, hint, say

BROADCAST = 0xFE
ATTEMPTS = 3


def parse_args(argv: List[str]) -> Tuple[Optional[str], int, int]:
    port, servo_id, bus = None, BROADCAST, 1
    args = list(argv)
    while args:
        arg = args.pop(0)
        if arg == "--id" and args and args[0].isdigit() and int(args[0]) <= 253:
            servo_id = int(args.pop(0))
        elif arg == "--bus" and args and args[0] in ("1", "2"):
            bus = int(args.pop(0))
        elif arg.startswith("/dev/") or arg.upper().startswith("COM"):
            port = arg
        else:
            sys.exit(__doc__)
    return port, servo_id, bus


def clean_reply(raw: List[int]) -> Optional[int]:
    """The servo ID if raw starts with a valid Herkulex STAT reply, else None."""
    if len(raw) < 9 or raw[0] != 0xFF or raw[1] != 0xFF or raw[2] != 9 or raw[4] != 0x47:
        return None
    x = raw[2] ^ raw[3] ^ raw[4] ^ raw[7] ^ raw[8]
    if raw[5] != (x & 0xFE) or raw[6] != (~x & 0xFE):
        return None
    return raw[3]


def sniff_once(bench, servo_id: int, bus: int) -> Optional[Tuple[int, List[int]]]:
    reply = bench.request(f"Sniff {servo_id} {bus}", timeout=2, vpid=ALL)
    if reply is None:
        say("FAIL: the ESP32 stopped answering.")
        return None
    if not reply.text.startswith("OK RAW"):
        say(f"FAIL: this firmware has no Sniff command ({reply.text}).")
        hint("Re-flash DBoxMotor from Urchin_Firmware:  pio run -e DBoxMotor -t upload")
        return None
    words = reply.text.split()          # OK RAW idle=N got=N XX XX ...
    idle = int(words[2].split("=")[1])
    raw = [int(b, 16) for b in words[4:]]
    return idle, raw


def main() -> int:
    port, servo_id, bus = parse_args(sys.argv[1:])
    bench = connect(port)
    if bench is None:
        return 1
    target = "broadcast (any servo)" if servo_id == BROADCAST else f"ID {servo_id}"
    target += f" on bus {bus}"
    say(f"\nPinging {target} {ATTEMPTS} times and recording the raw reply line...\n")

    results = []
    for attempt in range(1, ATTEMPTS + 1):
        got = sniff_once(bench, servo_id, bus)
        if got is None:
            return 1
        idle, raw = got
        shown = " ".join(f"{b:02X}" for b in raw) or "(nothing)"
        say(f"  try {attempt}: noise before ping: {idle} bytes   reply: {shown}")
        results.append((idle, raw))

    say("")
    ids = {clean_reply(raw) for _, raw in results} - {None}
    any_bytes = any(raw for _, raw in results)
    noise = any(idle for idle, _ in results)

    if ids:
        found = ", ".join(str(i) for i in sorted(ids))
        say(f"RESULT: clean replies from servo ID {found} -- the reply wire works.")
        hint("Run bench_test.py option 2; it should find it now.",
             "If option 2 still can't, tell the firmware team: the replies are fine,",
             "   so the problem is in how the firmware checks them.")
    elif any_bytes:
        say("RESULT: the servo IS answering, but the bytes arrive garbled.")
        hint("The reply wire is connected, but the signal is distorted. Check:",
             "   - signal level: measure servo TXD (pin 3) to GND; 5 V needs a",
             "     divider or level shifter before the ESP32's 3.3 V pin",
             "   - a long or loose jumper on that wire; keep it short",
             "   - GND shared between servo, supply and ESP32",
             "   - only ONE servo connected (two answering a broadcast collide);",
             "     or use --id <its ID> to ping just one")
    else:
        say("RESULT: nothing at all came back on the reply wire.")
        hint("The servo isn't sending, or its TXD isn't reaching ESP32 GPIO16. Check:",
             "   - continuity (supply off): servo connector pin 3 <-> the ESP32 pin",
             "     marked 16 / RX2, measured at the servo pin itself",
             "   - that wire is in the SAME breadboard row as the other servo's TXD",
             "   - pin order on the harness (manual page 17: 1 GND 2 VDD 3 TXD 4 RXD)")
    if noise:
        hint("Bytes arrived with nothing sent: the reply line is picking up noise",
             "   (floating or loose wire, or another device talking on it).")
    bench.link.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
