"""
MarshalType bench test -- drives the servos with real MarshalType Items (the
team's binary command format, Clam/MarshalType) instead of text commands.

    laptop/Pi: Item("10", "Move", [30.0, 1000])
        -> MarshalType packs it into bytes
        -> D-BOX carries them to the ESP32 (stream 'M')
    ESP32 (DBoxMotor v3):
        -> MarshalType unpacks the same Item
        -> the servo moves; the answer comes back like a text command's

The joint is the servo ID as text ("10") or "all": the bench has no bridge
list of joint names yet (see DBoxMotor/main.cpp).

Run from Crab_API, the same way as bench_test.py:

    sudo env PYTHONPATH=$HOME/dbox python3 src/marshal_test.py
    sudo env PYTHONPATH=$HOME/dbox python3 src/marshal_test.py /dev/ttyUSB1

Needs DBoxMotor v3 firmware on the ESP32, and both Python modules in
$HOME/dbox: DBOXpy (Clam/D-BOX) and marshaltype_py (Clam/MarshalType, see
BENCH_TEST.md for building it).
"""

import sys
import time
from typing import Optional

from bench_test import ALL, connect, dbox, hint, motor_name, parse_servo_list, say

try:
    import marshaltype_py as mt
except ImportError:
    sys.exit("marshaltype_py not found. Build Clam/MarshalType and copy\n"
             "build/python/marshaltype_py*.so into $HOME/dbox (see BENCH_TEST.md).")

ITEM_STREAM = 'M'
MAX_ANGLE = 160


def describe_bytes(data: bytes) -> None:
    """Show the packed Item the way MarshalType lays it out."""
    joint = data[:50].rstrip(b"\x00").decode(errors="replace")
    cmd_len, n_values = data[50], data[51]
    command = data[52:52 + cmd_len].decode(errors="replace")
    say(f"   MarshalType bytes ({len(data)}):")
    say(f"     joint   (50 B)  '{joint}' + {50 - len(joint)} zero bytes")
    say(f"     lengths ( 2 B)  {cmd_len:02X} {n_values:02X}   (command is {cmd_len} letters, "
        f"{n_values} value{'s' if n_values != 1 else ''})")
    say(f"     command ({cmd_len:2} B)  {data[52:52 + cmd_len].hex(' ').upper()}   '{command}'")
    offset = 52 + cmd_len
    for i in range(n_values):
        chunk = data[offset:offset + 5]
        say(f"     value {i + 1} ( 5 B)  {chunk.hex(' ').upper()}   tag '{chr(chunk[0])}'")
        offset += 5


def send_item(bench, item, timeout: float = 2.0, show: bool = True) -> Optional["dbox.DBOX"]:
    """Pack the Item, send it over D-BOX, return the ESP32's reply."""
    err, data = item.Marshal()
    if err != 0:
        say(f"FAIL: MarshalType could not pack the command (error {err}).")
        return None
    data = bytes(data)
    if show:
        describe_bytes(data)

    packet = dbox.DBOX()
    packet.VPID = ALL
    packet.stream = ITEM_STREAM
    packet.payload = data
    bench.link.proto.send(packet)

    # The reply comes back on stream 'E', on the target servo's VPID.
    deadline = time.monotonic() + timeout
    while True:
        reply = bench.link.receive(max(0.0, deadline - time.monotonic()))
        if reply is None:
            return None
        if reply.stream == 'D':
            who = f"motor {reply.VPID}" if reply.VPID else "ESP32"
            say(f"   ({who} says: {reply.text})")
            continue
        if reply.stream == 'E':
            return reply


def report(reply) -> None:
    if reply is None:
        say("FAIL: no reply from the ESP32.")
        hint("Is DBoxMotor v3 flashed? Older firmware ignores MarshalType packets.")
    elif reply.text.startswith("OK"):
        say(f"PASS: ESP32 unpacked it and answered: {reply.text}")
    else:
        say(f"FAIL: ESP32 refused it: {reply.text}")


def ask_joint(servos: dict) -> Optional[str]:
    if servos:
        say("   Motors: " + ", ".join(motor_name(i, m) for i, m in servos.items()))
    raw = input("Motor ID (or 'all'): ").strip().lower()
    if raw == "all" or (raw.isdigit() and 1 <= int(raw) <= 253):
        return raw
    say("   Enter a motor ID from 1 to 253, or 'all'.")
    return None


def move(bench, servos: dict) -> None:
    joint = ask_joint(servos)
    if joint is None:
        return
    raw = input(f"Angle in degrees (-{MAX_ANGLE} to {MAX_ANGLE}): ").strip()
    try:
        angle = float(raw)
    except ValueError:
        say("   That's not a number.")
        return
    if abs(angle) > MAX_ANGLE:
        say(f"   Keep it within +/-{MAX_ANGLE} degrees.")
        return
    raw = input("Travel time in ms [1000]: ").strip() or "1000"
    if not raw.isdigit():
        say("   Time must be a whole number of milliseconds.")
        return
    item = mt.Item(joint, "Move", [angle, int(raw)])
    say(f"\n Sending Item(joint='{joint}', command='Move', values=[{angle}, {int(raw)}])")
    report(send_item(bench, item))


def sync_move(bench, servos: dict) -> None:
    """Option A: one Item per joint to queue its target, then one Item to go."""
    if not servos:
        say("   No motors known yet -- run option 1 first.")
        return
    send_item(bench, mt.Item("all", "Clear", []), show=False)
    queued, need_shared = 0, False
    for servo_id, model in servos.items():
        raw = input(f"  {motor_name(servo_id, model)}: angle (blank = stay): ").strip()
        if not raw:
            continue
        try:
            angle = float(raw)
        except ValueError:
            say("   Not a number; skipping this motor.")
            continue
        own = input("    its own travel time in ms (blank = shared): ").strip()
        values = [angle, int(own)] if own.isdigit() else [angle]
        need_shared = need_shared or len(values) == 1
        say(f"\n Sending Item(joint='{servo_id}', command='Queue', values={values})")
        reply = send_item(bench, mt.Item(str(servo_id), "Queue", values), show=False)
        report(reply)
        if reply is None or not reply.text.startswith("OK"):
            send_item(bench, mt.Item("all", "Clear", []), show=False)
            return
        queued += 1
    if queued == 0:
        return
    go_values = []  # every motor has its own time: Go needs none
    if need_shared:
        shared = input("Shared travel time in ms [1000]: ").strip() or "1000"
        go_values = [int(shared) if shared.isdigit() else 1000]
    say(f"\n Sending Item(joint='all', command='Go', values={go_values})")
    report(send_item(bench, mt.Item("all", "Go", go_values)))


def find(bench) -> dict:
    say("\n Sending Item(joint='all', command='Find') -- scanning both buses (~20 s)...")
    reply = send_item(bench, mt.Item("all", "Find", []), timeout=35)
    report(reply)
    if reply is not None and reply.text.startswith("OK FOUND"):
        servos = parse_servo_list(reply.text[len("OK FOUND"):])
        for servo_id, model in servos.items():
            say(f"   {motor_name(servo_id, model)}")
        return servos
    return {}


def main() -> int:
    port = sys.argv[1] if len(sys.argv) > 1 else None
    bench = connect(port)
    if bench is None:
        return 1
    if "v3" not in bench.info.split():
        say(f"Firmware says: {bench.info}")
        hint("MarshalType needs DBoxMotor v3. Re-flash from Urchin_Firmware:",
             "   pio run -e DBoxMotor -t upload")
        return 1
    servos = dict(bench.servos)

    try:
        while True:
            say("\n 1) Check motors (Find, sent as an Item)")
            say(" 2) Move a motor (Item 'Move')")
            say(" 3) Stop all motors (Item 'Stop')")
            say(" 4) Show the bytes for a Move Item (nothing sent)")
            say(" 5) Synchronized move (Queue Items, then one Go Item)")
            say(" q) Quit")
            choice = input("Choose: ").strip().lower()
            if choice == '1':
                servos = find(bench) or servos
            elif choice == '2':
                move(bench, servos)
            elif choice == '3':
                say("\n Sending Item(joint='all', command='Stop')")
                report(send_item(bench, mt.Item("all", "Stop", [])))
            elif choice == '4':
                err, data = mt.Item("10", "Move", [30.0, 1000]).Marshal()
                say("\n Item(joint='10', command='Move', values=[30.0, 1000])")
                describe_bytes(bytes(data))
            elif choice == '5':
                sync_move(bench, servos)
            elif choice in ('q', 'quit', 'exit'):
                break
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        send_item(bench, mt.Item("all", "Stop", []), timeout=0.3, show=False)
        bench.link.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
