#!/usr/bin/env python3
"""Emulate HerkuleX servos on the Urchin simulator's motor bus.

The Urchin firmware drives its HerkuleX bus on UART1, which urchin-sim
exposes as a TCP server. This connects to that server and plays the part of
the servos: it tracks torque and LED state, moves each servo toward its jog
goal over the commanded play time, and answers reads with the live position.

Every event is logged with a ROS-style epoch timestamp
("[INFO] [1793195187.297660] [motor-sim]: ...") so the logs can be lined up
with Crab API and ROS node logs to measure latency.
"""

from __future__ import annotations

import argparse
import logging
import math
import os
import select
import socket
import sys
import time
import tomllib
from dataclasses import dataclass

log = logging.getLogger("motor-sim")

TICK_S = 0.0112  # HerkuleX play time unit (11.2 ms)
BROADCAST_ID = 0xFE
HEADER = b"\xff\xff"
MIN_PACKET_SIZE = 7  # header(2) + size + id + cmd + checksum1 + checksum2
MAX_PACKET_SIZE = 223

# Request commands. A servo's ACK echoes the command with ACK_BIT set.
EEP_WRITE = 0x01
EEP_READ = 0x02
RAM_WRITE = 0x03
RAM_READ = 0x04
I_JOG = 0x05
S_JOG = 0x06
STAT = 0x07
ROLLBACK = 0x08
REBOOT = 0x09
ACK_BIT = 0x40
CMD_NAMES = {
    EEP_WRITE: "EEP_WRITE",
    EEP_READ: "EEP_READ",
    RAM_WRITE: "RAM_WRITE",
    RAM_READ: "RAM_READ",
    I_JOG: "I_JOG",
    S_JOG: "S_JOG",
    STAT: "STAT",
    ROLLBACK: "ROLLBACK",
    REBOOT: "REBOOT",
}
READ_CMDS = {EEP_READ, RAM_READ, STAT}

# Register map (DRS-0101/0201/0601/0602 share the addresses used here).
REG_SIZE = 0x80
EEP_MODEL_1 = 0x00
EEP_MODEL_2 = 0x01
EEP_ID = 0x06
EEP_ACK_POLICY = 0x07
RAM_ID = 0x00
RAM_ACK_POLICY = 0x01
RAM_STATUS_ERROR = 0x30
RAM_STATUS_DETAIL = 0x31
RAM_TORQUE = 0x34
RAM_LED = 0x35
RAM_VOLTAGE = 0x36
RAM_TEMPERATURE = 0x37
RAM_CONTROL_MODE = 0x38
RAM_TICK = 0x39
RAM_CALIBRATED_POSITION = 0x3A
RAM_ABSOLUTE_POSITION = 0x3C
RAM_DIFFERENTIAL_POSITION = 0x3E
RAM_PWM = 0x40
RAM_ABSOLUTE_2ND_POSITION = 0x42
RAM_GOAL_POSITION = 0x44
RAM_TRAJECTORY_POSITION = 0x46
RAM_DESIRED_VELOCITY = 0x48
POSITION_REGS = {
    RAM_CALIBRATED_POSITION,
    RAM_ABSOLUTE_POSITION,
    RAM_ABSOLUTE_2ND_POSITION,
}

TORQUE_FREE = 0x00
TORQUE_BRAKE = 0x40
TORQUE_ON = 0x60
TORQUE_NAMES = {TORQUE_FREE: "FREE", TORQUE_BRAKE: "BRAKE", TORQUE_ON: "ON"}

ERROR_EXCEED_POSITION = 0x02
ERROR_INVALID_PACKET = 0x08
DETAIL_MOVING = 0x01
DETAIL_INPOSITION = 0x02
DETAIL_CHECKSUM_ERROR = 0x04
DETAIL_UNKNOWN_COMMAND = 0x08
DETAIL_EXCEED_REG_RANGE = 0x10
DETAIL_MOTOR_ON = 0x40

# Rough idle readings: ~7.4 V (0.074 V/unit) and ~30 C.
IDLE_VOLTAGE = 100
IDLE_TEMPERATURE = 30

LED_NAMES = ("green", "blue", "red")


@dataclass(frozen=True)
class Model:
    """A HerkuleX servo model.

    The degree range and 0.1 degree rounding are the Urchin firmware's
    raw-to-degree mapping (HerkuleXMotorBounds in UnPacker.cpp and
    PointToAngle in LimitChecks.c), so logged angles match what the Crab API
    reports.
    """

    name: str
    raw_max: int
    angle_min: float
    angle_max: float
    eep_model: tuple[int, int]

    def to_degrees(self, raw: float) -> float:
        span = self.angle_max - self.angle_min
        angle = self.angle_min + raw / self.raw_max * span
        # C round(): halves go away from zero.
        return math.copysign(math.floor(abs(angle) * 10 + 0.5) / 10, angle)


MODELS = {
    "0101": Model("0101", 1023, -166.7, 166.7, (0x01, 0x01)),
    "0201": Model("0201", 1023, -166.7, 166.7, (0x02, 0x01)),
    "0601": Model("0601", 2047, -166.7, 166.6, (0x06, 0x01)),
    "0602": Model("0602", 32767, -456.2, 456.2, (0x06, 0x02)),
}


@dataclass
class Move:
    start: float
    goal: int
    t0: float
    duration: float

    @property
    def end(self) -> float:
        return self.t0 + self.duration

    def position_at(self, now: float) -> float:
        if self.duration <= 0 or now >= self.end:
            return self.goal
        frac = max(0.0, (now - self.t0) / self.duration)
        return self.start + (self.goal - self.start) * frac


def checksums(size: int, sid: int, cmd: int, data: bytes) -> tuple[int, int]:
    xor = size ^ sid ^ cmd
    for b in data:
        xor ^= b
    return xor & 0xFE, ~xor & 0xFE


def packet(sid: int, cmd: int, data: bytes) -> bytes:
    size = MIN_PACKET_SIZE + len(data)
    ck1, ck2 = checksums(size, sid, cmd, data)
    return bytes([0xFF, 0xFF, size, sid, cmd, ck1, ck2]) + data


class Servo:
    """State of one emulated servo."""

    def __init__(self, sid: int, model: Model, name: str | None, now: float):
        self.id = sid
        self.model = model
        self.label = f"servo {sid} ({name})" if name else f"servo {sid}"
        self.eep = bytearray(REG_SIZE)
        self.ram = bytearray(REG_SIZE)
        self.factory_reset()
        self.position = (model.raw_max + 1) // 2
        self.move: Move | None = None
        self.reboot(now, quiet=True)

    # -- registers ----------------------------------------------------------

    def factory_reset(self) -> None:
        self.eep[:] = bytes(REG_SIZE)
        self.eep[EEP_MODEL_1], self.eep[EEP_MODEL_2] = self.model.eep_model
        self.eep[EEP_ID] = self.id
        self.eep[EEP_ACK_POLICY] = 1

    def reboot(self, now: float, quiet: bool = False) -> None:
        """Reload RAM from EEP: torque free, errors cleared, motion stopped."""
        self.stop(now, reason="reboot", quiet=quiet)
        self.ram[:] = bytes(REG_SIZE)
        self.ram[RAM_ID] = self.eep[EEP_ID]
        self.ram[RAM_ACK_POLICY] = self.eep[EEP_ACK_POLICY]
        self.ram[RAM_VOLTAGE] = IDLE_VOLTAGE
        self.ram[RAM_TEMPERATURE] = IDLE_TEMPERATURE

    @property
    def torque(self) -> int:
        return self.ram[RAM_TORQUE]

    @property
    def ack_policy(self) -> int:
        return self.ram[RAM_ACK_POLICY]

    def status(self, now: float) -> bytes:
        detail = self.ram[RAM_STATUS_DETAIL] & ~(
            DETAIL_MOVING | DETAIL_INPOSITION | DETAIL_MOTOR_ON
        )
        if self.moving(now):
            detail |= DETAIL_MOVING
        else:
            detail |= DETAIL_INPOSITION
        if self.torque == TORQUE_ON:
            detail |= DETAIL_MOTOR_ON
        return bytes([self.ram[RAM_STATUS_ERROR], detail])

    def flag_error(self, error: int, detail: int = 0) -> None:
        self.ram[RAM_STATUS_ERROR] |= error
        self.ram[RAM_STATUS_DETAIL] |= detail

    def read_ram(self, addr: int, n: int, now: float) -> bytes:
        regs = bytearray(self.ram)
        pos = round(self.position_at(now))
        goal = self.move.goal if self.move else pos
        velocity = 0
        if self.moving(now):
            velocity = round(
                (self.move.goal - self.move.start) / self.move.duration * TICK_S
            )
        for reg in POSITION_REGS | {RAM_TRAJECTORY_POSITION}:
            regs[reg : reg + 2] = pos.to_bytes(2, "little")
        regs[RAM_GOAL_POSITION : RAM_GOAL_POSITION + 2] = goal.to_bytes(2, "little")
        regs[RAM_DIFFERENTIAL_POSITION : RAM_DIFFERENTIAL_POSITION + 2] = (
            velocity.to_bytes(2, "little", signed=True)
        )
        regs[RAM_TICK] = int(now / TICK_S) & 0xFF
        regs[RAM_STATUS_ERROR : RAM_STATUS_DETAIL + 1] = self.status(now)

        if any(addr <= reg < addr + n for reg in POSITION_REGS):
            note = " (moving)" if self.moving(now) else ""
            log.info(
                "%s: read position %.1f deg (raw %d)%s",
                self.label,
                self.model.to_degrees(pos),
                pos,
                note,
            )
        else:
            log.debug("%s: read RAM 0x%02X..0x%02X", self.label, addr, addr + n - 1)
        return bytes(regs[addr : addr + n])

    def write_ram(
        self, addr: int, data: bytes, now: float, quiet: bool = False
    ) -> None:
        for offset, value in enumerate(data):
            reg = addr + offset
            if reg == RAM_TORQUE:
                self.set_torque(value, now, quiet)
            elif reg == RAM_ACK_POLICY and value != self.ack_policy:
                if not quiet:
                    log.info(
                        "%s: ACK policy %d -> %d", self.label, self.ack_policy, value
                    )
            elif reg in (RAM_STATUS_ERROR, RAM_STATUS_DETAIL):
                if not quiet and self.ram[reg]:
                    log.info(
                        "%s: cleared status %s",
                        self.label,
                        "error" if reg == RAM_STATUS_ERROR else "detail",
                    )
            elif reg == RAM_LED:
                self.set_led(value, quiet)
            self.ram[reg] = value

    def set_torque(self, value: int, now: float, quiet: bool) -> None:
        if value == self.torque:
            return
        if not quiet:
            log.info(
                "%s: torque %s -> %s",
                self.label,
                TORQUE_NAMES.get(self.torque, hex(self.torque)),
                TORQUE_NAMES.get(value, hex(value)),
            )
        if value != TORQUE_ON:
            self.stop(now, reason="torque off", quiet=quiet)

    def set_led(self, value: int, quiet: bool = False) -> None:
        if value != self.ram[RAM_LED] and not quiet:
            on = [n for i, n in enumerate(LED_NAMES) if value & (1 << i)]
            log.debug("%s: LED %s", self.label, "+".join(on) or "off")
        self.ram[RAM_LED] = value

    # -- motion -------------------------------------------------------------

    def position_at(self, now: float) -> float:
        return self.move.position_at(now) if self.move else self.position

    def moving(self, now: float) -> bool:
        return self.move is not None and now < self.move.end

    def deg(self, raw: float) -> str:
        return f"{self.model.to_degrees(raw):.1f} deg"

    def stop(self, now: float, reason: str, quiet: bool = False) -> None:
        if self.move is None:
            return
        self.position = self.position_at(now)
        if not quiet and now < self.move.end:
            log.info(
                "%s: stopped at %s (%s)", self.label, self.deg(self.position), reason
            )
        self.move = None

    def jog(self, goal: int, ticks: int, set_byte: int, now: float, cmd: str) -> None:
        self.set_led((set_byte >> 2) & 0x07)
        if set_byte & 0x02:
            log.warning(
                "%s: %s in velocity mode is not emulated; ignored", self.label, cmd
            )
            return
        if set_byte & 0x01:
            self.stop(now, reason=f"{cmd} stop bit")
            return
        if self.torque != TORQUE_ON:
            log.warning(
                "%s: %s to raw %d ignored, torque is %s",
                self.label,
                cmd,
                goal,
                TORQUE_NAMES.get(self.torque, hex(self.torque)),
            )
            return
        if goal > self.model.raw_max:
            log.warning(
                "%s: %s goal raw %d is past the DRS-%s limit %d",
                self.label,
                cmd,
                goal,
                self.model.name,
                self.model.raw_max,
            )
            self.flag_error(ERROR_EXCEED_POSITION)
            return

        start = self.position_at(now)
        if self.moving(now):
            log.info(
                "%s: move to %s interrupted at %s",
                self.label,
                self.deg(self.move.goal),
                self.deg(start),
            )
        self.position = start
        self.move = Move(start, goal, now, ticks * TICK_S)
        log.info(
            "%s: MOVE %s -> %s (raw %d -> %d) over %.3f s [%s]",
            self.label,
            self.deg(start),
            self.deg(goal),
            round(start),
            goal,
            self.move.duration,
            cmd,
        )
        self.update(now)

    def update(self, now: float) -> None:
        """Finish the current move once its play time has elapsed."""
        if self.move is None or now < self.move.end:
            return
        move, self.move = self.move, None
        self.position = move.goal
        log.info(
            "%s: REACHED %s after %.3f s",
            self.label,
            self.deg(move.goal),
            now - move.t0,
        )

    def next_event(self) -> float | None:
        return self.move.end if self.move else None


class Bus:
    """Parses HerkuleX packets and dispatches them to the emulated servos."""

    def __init__(
        self, servos: dict[int, Servo], auto_model: Model | None, strict_acks: bool
    ):
        self.servos = servos
        self.auto_model = auto_model
        self.strict_acks = strict_acks
        self.buf = bytearray()
        self.broadcast_writes: list[tuple[int, bytes]] = []
        self.missing_warned: set[int] = set()

    def reset_link(self) -> None:
        self.buf.clear()

    def servo(self, sid: int, now: float) -> Servo | None:
        if sid in self.servos:
            return self.servos[sid]
        if self.auto_model is None:
            if sid not in self.missing_warned:
                self.missing_warned.add(sid)
                log.warning(
                    "no servo with ID %d on the bus; ignoring packets for it", sid
                )
            return None
        servo = Servo(sid, self.auto_model, None, now)
        for addr, data in self.broadcast_writes:
            servo.write_ram(addr, data, now, quiet=True)
        self.servos[sid] = servo
        log.info(
            "%s: discovered, emulating as DRS-%s (torque %s)",
            servo.label,
            self.auto_model.name,
            TORQUE_NAMES.get(servo.torque, hex(servo.torque)),
        )
        return servo

    def feed(self, chunk: bytes, now: float) -> bytes:
        """Consume bytes from the firmware; return bytes to send back."""
        self.buf += chunk
        replies = bytearray()
        while True:
            start = self.buf.find(HEADER)
            if start < 0:
                keep = 1 if self.buf.endswith(b"\xff") else 0
                if len(self.buf) > keep:
                    log.debug("discarding %d stray bytes", len(self.buf) - keep)
                del self.buf[: len(self.buf) - keep]
                break
            if start:
                log.debug("discarding %d stray bytes", start)
                del self.buf[:start]
            if len(self.buf) < 3:
                break
            size = self.buf[2]
            if not MIN_PACKET_SIZE <= size <= MAX_PACKET_SIZE:
                log.warning("bad packet size %d; resyncing", size)
                del self.buf[0]
                continue
            if len(self.buf) < size:
                break
            pkt = bytes(self.buf[:size])
            del self.buf[:size]
            replies += self.handle(pkt, now)
        return bytes(replies)

    def handle(self, pkt: bytes, now: float) -> bytes:
        size, sid, cmd, ck1, ck2 = pkt[2:7]
        data = pkt[7:]
        name = CMD_NAMES.get(cmd, f"0x{cmd:02X}")
        log.debug("rx %s id=%d data=[%s]", name, sid, data.hex(" "))

        if (ck1, ck2) != checksums(size, sid, cmd, data):
            log.warning("%s for id %d has a bad checksum; dropped", name, sid)
            if sid in self.servos:
                self.servos[sid].flag_error(ERROR_INVALID_PACKET, DETAIL_CHECKSUM_ERROR)
            return b""

        if cmd in (I_JOG, S_JOG):
            self.jog(cmd, data, now)
            target = self.servos.get(sid) if sid != BROADCAST_ID else None
            return self.ack(target, cmd, b"", now)

        if sid == BROADCAST_ID:
            if cmd == RAM_WRITE and len(data) >= 2:
                self.broadcast_writes.append((data[0], data[2 : 2 + data[1]]))
            for servo in list(self.servos.values()):
                self.execute(servo, cmd, data, now)
            return b""

        servo = self.servo(sid, now)
        if servo is None:
            return b""
        payload = self.execute(servo, cmd, data, now)
        if payload is None:
            return b""
        return self.ack(servo, cmd, payload, now)

    def execute(self, servo: Servo, cmd: int, data: bytes, now: float) -> bytes | None:
        """Run a non-jog command; return the ACK payload or None."""
        if cmd in (RAM_READ, EEP_READ, RAM_WRITE, EEP_WRITE):
            if len(data) < 2:
                log.warning("%s: short %s packet", servo.label, CMD_NAMES[cmd])
                servo.flag_error(ERROR_INVALID_PACKET)
                return None
            addr, n = data[0], data[1]
            if addr + n > REG_SIZE:
                log.warning(
                    "%s: %s 0x%02X+%d is past the register map",
                    servo.label,
                    CMD_NAMES[cmd],
                    addr,
                    n,
                )
                servo.flag_error(ERROR_INVALID_PACKET, DETAIL_EXCEED_REG_RANGE)
                return None
            if cmd == RAM_READ:
                return bytes([addr, n]) + servo.read_ram(addr, n, now)
            if cmd == EEP_READ:
                log.debug(
                    "%s: read EEP 0x%02X..0x%02X", servo.label, addr, addr + n - 1
                )
                return bytes([addr, n]) + bytes(servo.eep[addr : addr + n])
            values = data[2 : 2 + n]
            if len(values) < n:
                log.warning(
                    "%s: %s declares %d bytes but carries %d",
                    servo.label,
                    CMD_NAMES[cmd],
                    n,
                    len(values),
                )
                servo.flag_error(ERROR_INVALID_PACKET)
                return None
            if cmd == RAM_WRITE:
                servo.write_ram(addr, values, now)
            else:
                log.info(
                    "%s: EEP write 0x%02X = [%s]", servo.label, addr, values.hex(" ")
                )
                servo.eep[addr : addr + n] = values
            return b""
        if cmd == STAT:
            log.debug("%s: STAT", servo.label)
            return b""
        if cmd == REBOOT:
            log.info("%s: REBOOT", servo.label)
            servo.reboot(now)
            return b""
        if cmd == ROLLBACK:
            log.info("%s: ROLLBACK to factory defaults", servo.label)
            servo.factory_reset()
            return b""
        log.warning("%s: unknown command 0x%02X", servo.label, cmd)
        servo.flag_error(ERROR_INVALID_PACKET, DETAIL_UNKNOWN_COMMAND)
        return None

    def jog(self, cmd: int, data: bytes, now: float) -> None:
        name = CMD_NAMES[cmd]
        if cmd == S_JOG:
            if not data:
                log.warning("empty S_JOG packet")
                return
            ticks, entries, width = data[0], data[1:], 4
        else:
            ticks, entries, width = None, data, 5
        if len(entries) % width:
            log.warning(
                "%s carries %d bytes, not a multiple of %d", name, len(entries), width
            )
        for i in range(0, len(entries) - width + 1, width):
            lsb, msb, set_byte, sid = entries[i : i + 4]
            servo_ticks = entries[i + 4] if ticks is None else ticks
            servo = self.servo(sid, now)
            if servo is not None:
                servo.jog(lsb | (msb << 8), servo_ticks, set_byte, now, name)

    def ack(self, servo: Servo | None, cmd: int, payload: bytes, now: float) -> bytes:
        if servo is None:
            return b""
        if self.strict_acks:
            policy = servo.ack_policy
            if policy == 0 or (policy == 1 and cmd not in READ_CMDS):
                return b""
            return packet(servo.id, cmd | ACK_BIT, payload + servo.status(now))
        # The current firmware only reads replies to reads, and for RAM/EEP
        # reads it stops before the two status bytes, so leave them off.
        if cmd not in READ_CMDS:
            return b""
        status = servo.status(now) if cmd == STAT else b""
        return packet(servo.id, cmd | ACK_BIT, payload + status)

    def update(self, now: float) -> None:
        for servo in self.servos.values():
            servo.update(now)

    def next_event(self) -> float | None:
        events = [t for s in self.servos.values() if (t := s.next_event()) is not None]
        return min(events, default=None)


def load_bridge(path: str, default_model: Model, now: float) -> dict[int, Servo]:
    with open(path, "rb") as f:
        bridge = tomllib.load(f)
    servos = {}
    for motor in bridge.get("Motor", []):
        sid, brand = motor.get("MotorNum"), motor.get("MotorBrand")
        name, model_name = motor.get("JointName"), motor.get("MotorModel")
        if brand != "HerkuleX":
            log.warning("bridge motor %s (%s) is not HerkuleX; skipped", sid, name)
            continue
        model = MODELS.get(model_name)
        if model is None:
            log.warning(
                "bridge motor %s (%s) has unknown model %r; using DRS-%s",
                sid,
                name,
                model_name,
                default_model.name,
            )
            model = default_model
        servos[sid] = Servo(sid, model, name, now)
    return servos


def connect(host: str, port: int, timeout: float | None) -> socket.socket | None:
    deadline = None if timeout is None else time.monotonic() + timeout
    waiting = False
    while True:
        try:
            sock = socket.create_connection((host, port), timeout=1)
        except OSError:
            if deadline is not None and time.monotonic() > deadline:
                return None
            if not waiting:
                log.info("waiting for the motor bus at %s:%d", host, port)
                waiting = True
            time.sleep(0.2)
            continue
        sock.setblocking(False)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        return sock


def serve(sock: socket.socket, bus: Bus) -> None:
    while True:
        now = time.monotonic()
        next_event = bus.next_event()
        timeout = 0.5 if next_event is None else max(0.0, next_event - now)
        readable, _, _ = select.select([sock], [], [], timeout)
        now = time.monotonic()
        if readable:
            chunk = sock.recv(4096)
            if not chunk:
                return
            reply = bus.feed(chunk, now)
            if reply:
                sock.sendall(reply)
        bus.update(time.monotonic())


class RosStyleFormatter(logging.Formatter):
    def format(self, record: logging.LogRecord) -> str:
        return (
            f"[{record.levelname}] [{record.created:.6f}] "
            f"[{record.name}]: {record.getMessage()}"
        )


def setup_logging(verbose: bool) -> None:
    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(RosStyleFormatter())
    # urchin-sim runs QEMU with -nographic, which puts the terminal in raw
    # mode, so a bare "\n" would not return the cursor to column 0.
    if sys.stdout.isatty():
        handler.terminator = "\r\n"
    log.addHandler(handler)
    log.setLevel(logging.DEBUG if verbose else logging.INFO)
    sys.stdout.reconfigure(line_buffering=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--host", default="127.0.0.1", help="simulator host (default: %(default)s)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("URCHIN_SIM_UART1_PORT", 5556)),
        help="simulator UART1 TCP port (default: $URCHIN_SIM_UART1_PORT or 5556)",
    )
    parser.add_argument(
        "--bridge",
        help="bridge.toml listing the servos on the bus; without it, any ID "
        "the firmware addresses is emulated as --model",
    )
    parser.add_argument(
        "--model",
        choices=sorted(MODELS),
        default="0601",
        help="model for servos not in a bridge file (default: %(default)s)",
    )
    parser.add_argument(
        "--strict-acks",
        action="store_true",
        help="reply exactly like real servos (honour the ACK policy and "
        "append status bytes); the current firmware cannot parse this",
    )
    parser.add_argument(
        "--exit-on-disconnect",
        action="store_true",
        help="exit when the simulator closes the bus instead of reconnecting",
    )
    parser.add_argument(
        "--connect-timeout",
        type=float,
        help="give up if the bus is not reachable within "
        "this many seconds (default: wait forever)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="also log raw packets and register reads",
    )
    args = parser.parse_args()
    setup_logging(args.verbose)

    now = time.monotonic()
    default_model = MODELS[args.model]
    if args.bridge:
        servos = load_bridge(args.bridge, default_model, now)
        auto_model = None
        for servo in servos.values():
            log.info(
                "%s: DRS-%s at %s",
                servo.label,
                servo.model.name,
                servo.deg(servo.position),
            )
    else:
        servos, auto_model = {}, default_model
        log.info(
            "no bridge file; emulating every addressed servo as DRS-%s",
            default_model.name,
        )
    log.info(
        "ACK mode: %s",
        "strict (real servo replies)" if args.strict_acks else "firmware-compatible",
    )
    bus = Bus(servos, auto_model, args.strict_acks)

    try:
        while True:
            sock = connect(args.host, args.port, args.connect_timeout)
            if sock is None:
                log.error(
                    "motor bus at %s:%d not reachable after %.0f s",
                    args.host,
                    args.port,
                    args.connect_timeout,
                )
                return 1
            log.info("connected to the motor bus at %s:%d", args.host, args.port)
            with sock:
                serve(sock, bus)
            log.warning("motor bus disconnected")
            if args.exit_on_disconnect:
                return 0
            bus.reset_link()
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    sys.exit(main())
