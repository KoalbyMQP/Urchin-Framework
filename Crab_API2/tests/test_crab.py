"""Crab client tests against a scripted fake firmware.

The fake answers the way Urchin_Firmware/src/Conversation/UnPacker.cpp does:
FormatTicket -> E: i32 error [, u32 ticket]; LoadTicket TestReact -> R reaction;
GetHealth FreeRam/TotalRam -> E text; GetHealth AckCheck -> D lines.
"""

import logging
import struct
import threading
from pathlib import Path

import pytest

from crab_api2 import herkulex
from crab_api2.bridge import load_bridge
from crab_api2.crab import (
    Crab,
    CrabError,
    MotorStatus,
    NoFreeTicketError,
    ReplyTimeoutError,
    TicketType,
    UnknownJointError,
)
from crab_api2.herkulex import HerkulexCommand, JogLedColor
from crab_api2.link import DATA_SIZE, START_MARKER, Link
from crab_api2.reaction import Reaction, ReactionEvent

BRIDGE = Path(__file__).parent / "fixtures" / "bridge.toml"  # Shoulder = 7, Hip = 8
TIMEOUT = 2.0


def frame(stream: str, data: bytes) -> bytes:
    return START_MARKER + b"\x00" + stream.encode() + data.ljust(DATA_SIZE, b"\0")


def reaction_data(ticket: int, joint: bytes, value: int, code: int = 45) -> bytes:
    command = b"TestReact"
    return (
        struct.pack("<II", ticket, code)
        + joint.ljust(50, b"\0")
        + bytes([len(command), 1])
        + command
        + b"i"
        + struct.pack("<i", value)
    )


class FakeFirmware:
    """A pyserial-like port backed by a scripted firmware."""

    timeout = 0.02

    def __init__(self) -> None:
        self.payloads: list[bytes] = []
        self.next_ticket = 0
        self.format_error = 0
        self.silent = False
        self.ack_lines: list[bytes] = []
        self.closed = False
        self._buffer = bytearray()
        self._cv = threading.Condition()

    # --- scripting ---

    def inject(self, data: bytes) -> None:
        with self._cv:
            self._buffer += data
            self._cv.notify_all()

    def respond(self, payload: bytes) -> bytes:
        if self.silent:
            return b""
        if payload.startswith(b"FormatTicket"):
            if self.format_error:
                return frame("E", struct.pack("<i", self.format_error))
            ticket = self.next_ticket
            self.next_ticket += 1
            return frame("E", struct.pack("<i", 0)) + frame(
                "E", struct.pack("<I", ticket)
            )
        if payload.startswith(b"LoadTicket"):
            ticket = struct.unpack_from("<I", payload, 10)[0]
            joint = payload[14:64].rstrip(b"\0")
            name_len = payload[64]
            name = payload[66 : 66 + name_len]
            if name == b"TestReact":
                value = struct.unpack_from("<i", payload, 66 + name_len + 1)[0]
                return frame("R", reaction_data(ticket, joint, value))
            return frame("D", b"LoadTicket called")
        if payload.startswith(b"GetHealthFreeRam\0"):
            return frame("E", b"123456\0")
        if payload.startswith(b"GetHealthTotalRam\0"):
            return frame("E", b"98765\0")
        if payload.startswith(b"GetHealthAckCheck\0"):
            lines = [b"GetHealth:AckCheck", *self.ack_lines, b"AckCheck:done"]
            return b"".join(frame("D", line + b"\0") for line in lines)
        return b""

    def sent(self, prefix: bytes) -> list[bytes]:
        return [p for p in self.payloads if p.startswith(prefix)]

    # --- pyserial surface used by Link ---

    def write(self, data: bytes) -> int:
        assert data[:1] == START_MARKER and len(data) == 3 + DATA_SIZE
        payload = bytes(data[3:])
        self.payloads.append(payload)
        self.inject(self.respond(payload))
        return len(data)

    def read_until(self, expected: bytes = b"\n", size: int | None = None) -> bytes:
        with self._cv:
            self._cv.wait_for(lambda: expected in self._buffer, timeout=self.timeout)
            index = self._buffer.find(expected)
            end = len(self._buffer) if index == -1 else index + len(expected)
            chunk = bytes(self._buffer[:end])
            del self._buffer[:end]
            return chunk

    def read(self, size: int = 1) -> bytes:
        with self._cv:
            self._cv.wait_for(lambda: len(self._buffer) >= size, timeout=self.timeout)
            chunk = bytes(self._buffer[:size])
            del self._buffer[:size]
            return chunk

    def close(self) -> None:
        self.closed = True


@pytest.fixture
def fake(monkeypatch):
    firmware = FakeFirmware()
    opened_with = []

    def fake_open(cls, url=None):
        opened_with.append(url)
        return cls(firmware)

    monkeypatch.setattr(Link, "open", classmethod(fake_open))
    firmware.opened_with = opened_with
    return firmware


@pytest.fixture
def crab(fake):
    with Crab(BRIDGE, reply_timeout=0.5) as client:
        yield client


def padded(packet: bytes) -> bytes:
    return packet.ljust(DATA_SIZE, b"\0")


# --- startup and shutdown ---


def test_bridge_uploaded_on_connect(fake, crab):
    shoulder, hip = load_bridge(BRIDGE)
    assert fake.sent(b"BridgeAdd") == [
        padded(shoulder.pack(False)),
        padded(hip.pack(True)),
    ]


def test_url_passed_to_link(fake):
    with Crab(BRIDGE, "socket://127.0.0.1:5555"):
        pass
    assert fake.opened_with == ["socket://127.0.0.1:5555"]


def test_motors_exposed(crab):
    assert [m.joint for m in crab.motors] == ["Shoulder", "Hip"]


def test_close_stops_threads_and_closes_link(fake):
    crab = Crab(BRIDGE)
    threads = [crab._reader, crab._dispatcher]
    crab.close()
    assert fake.closed
    assert not any(t.is_alive() for t in threads)
    crab.close()  # idempotent


# --- send ---


def test_send_ticket_sequence(fake, crab):
    command = herkulex.move_one("Shoulder", -23.0, 40, JogLedColor.LED_BLUE)
    ticket = crab.send(TicketType.ASYNCHRONOUS, [command])
    assert ticket == 0
    after_bridge = fake.payloads[2:]
    assert after_bridge == [
        padded(b"FormatTicketA"),
        padded(command.pack(0)),
        padded(b"PunchTicket" + struct.pack("<I", 0)),
    ]


def test_send_multiple_commands_in_order(fake, crab):
    commands = [herkulex.get_angle("Shoulder"), herkulex.get_angle("Hip")]
    crab.send(TicketType.SEQUENTIAL, commands)
    assert fake.sent(b"LoadTicket") == [padded(c.pack(0)) for c in commands]


def test_ticket_types():
    assert [t.value for t in TicketType] == ["S", "R", "A", "I"]


def test_tickets_increment(crab):
    tickets = [
        crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])
        for _ in range(3)
    ]
    assert tickets == [0, 1, 2]


def test_unknown_joint_rejected_before_sending(fake, crab):
    before = list(fake.payloads)
    with pytest.raises(UnknownJointError, match="'Elbow' is not in the bridge"):
        crab.send(
            TicketType.ASYNCHRONOUS,
            [herkulex.get_angle("Hip"), herkulex.get_angle("Elbow")],
        )
    assert fake.payloads == before


def test_no_free_ticket(fake, crab):
    fake.format_error = 1
    with pytest.raises(NoFreeTicketError):
        crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])
    assert fake.sent(b"LoadTicket") == []


def test_other_format_error(fake, crab):
    fake.format_error = 5
    with pytest.raises(CrabError, match="FormatTicket failed with error code 5"):
        crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])


def test_reply_timeout(fake, crab):
    fake.silent = True
    with pytest.raises(ReplyTimeoutError):
        crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])


def test_errors_are_crab_errors():
    for error in (NoFreeTicketError, ReplyTimeoutError, UnknownJointError):
        assert issubclass(error, CrabError)


def test_stale_reply_is_discarded(fake, crab):
    # A late reply from an earlier, timed-out request must not be taken as this ticket
    fake.inject(frame("E", struct.pack("<I", 999)))
    threading.Event().wait(0.1)  # let the reader queue it
    fake.next_ticket = 4
    assert crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")]) == 4


def test_concurrent_sends_get_distinct_tickets(crab):
    results = []

    def worker():
        for _ in range(5):
            results.append(
                crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])
            )

    threads = [threading.Thread(target=worker) for _ in range(4)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    assert sorted(results) == list(range(20))


# --- reactions ---


def test_reaction_round_trip(crab):
    received: list[ReactionEvent] = []
    done = threading.Event()

    def on_react(event: ReactionEvent) -> None:
        received.append(event)
        done.set()

    reaction = Reaction("Shoulder", HerkulexCommand.TEST_REACT, [45], on_react)
    ticket = crab.send(
        TicketType.ASYNCHRONOUS, [herkulex.test_react("Shoulder", 69)], [reaction]
    )
    assert done.wait(TIMEOUT)
    assert received == [ReactionEvent(ticket, 45, "Shoulder", "TestReact", (69,))]


def test_callback_can_send_without_deadlock(crab):
    follow_up: list[int] = []
    done = threading.Event()

    def on_react(event: ReactionEvent) -> None:
        follow_up.append(
            crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")])
        )
        done.set()

    reaction = Reaction("Shoulder", "TestReact", [45], on_react)
    crab.send(TicketType.ASYNCHRONOUS, [herkulex.test_react("Shoulder", 1)], [reaction])
    assert done.wait(TIMEOUT)
    assert follow_up == [1]


def test_close_ticket_removes_reactions(fake, crab):
    received: list[ReactionEvent] = []
    got = threading.Event()

    def on_react(event: ReactionEvent) -> None:
        received.append(event)
        got.set()

    reaction = Reaction("Shoulder", "TestReact", [45], on_react)
    ticket = crab.send(
        TicketType.ASYNCHRONOUS, [herkulex.test_react("Shoulder", 1)], [reaction]
    )
    assert got.wait(TIMEOUT)
    crab.close_ticket(ticket)

    # Prove the dispatcher is still alive and processing, then check nothing new arrived
    marker = threading.Event()
    crab._reactions.add(500, [Reaction("Hip", "TestReact", [], lambda e: marker.set())])
    fake.inject(frame("R", reaction_data(ticket, b"Shoulder", 2)))
    fake.inject(frame("R", reaction_data(500, b"Hip", 3)))
    assert marker.wait(TIMEOUT)
    assert len(received) == 1


def test_malformed_reaction_is_logged(fake, crab, caplog):
    with caplog.at_level(logging.WARNING, logger="crab_api2.crab"):
        # One value with an unknown 'q' tag
        fake.inject(
            frame(
                "R", struct.pack("<II", 0, 0) + b"Hip".ljust(50, b"\0") + b"\x01\x01Xq"
            )
        )
        # Reader must survive and keep serving requests
        assert crab.send(TicketType.ASYNCHRONOUS, [herkulex.get_angle("Hip")]) == 0
    assert "unknown value type" in caplog.text


def test_ticket_info_not_implemented(crab):
    with pytest.raises(NotImplementedError):
        crab.ticket_info(0)


# --- health ---


def test_free_ram(fake, crab):
    assert crab.get_health("FreeRam") == 123456
    assert fake.sent(b"GetHealth") == [padded(b"GetHealthFreeRam")]


def test_total_ram(crab):
    assert crab.get_health("TotalRam") == 98765


@pytest.mark.parametrize("check", ["CPU", "freeram", ""])
def test_unsupported_health_check(crab, check):
    with pytest.raises(ValueError, match="FreeRam, TotalRam, AckCheck"):
        crab.get_health(check)  # type: ignore[arg-type]


def test_ack_check(fake, crab):
    fake.ack_lines = [
        b"GetHealth:AckCheck:Checking 7",
        b"HF7\x01\x02",
        b"GetHealth:AckCheck:Checking 8",
        b"HM800",
    ]
    assert crab.get_health("AckCheck") == {
        "Shoulder": MotorStatus(
            "Shoulder", 7, responding=True, status_detail=1, status_error=2
        ),
        "Hip": MotorStatus("Hip", 8, responding=False, status_detail=0, status_error=0),
    }


def test_ack_check_status_bytes_that_look_like_digits(fake, crab):
    fake.ack_lines = [b"GetHealth:AckCheck:Checking 7", b"HF7" + b"12"]  # 0x31, 0x32
    status = crab.get_health("AckCheck")["Shoulder"]
    assert (status.status_detail, status.status_error) == (0x31, 0x32)


def test_ack_check_zero_status_truncates_line(fake, crab):
    # "%c" with a 0 byte ends the C string, so the line is just "HF7"
    fake.ack_lines = [b"GetHealth:AckCheck:Checking 7", b"HF7"]
    status = crab.get_health("AckCheck")["Shoulder"]
    assert status == MotorStatus("Shoulder", 7, True, 0, 0)


def test_ack_check_timeout(fake, crab):
    fake.silent = True
    with pytest.raises(ReplyTimeoutError):
        crab.get_health("AckCheck")


# --- logging ---


def test_firmware_debug_logged(fake, crab, caplog):
    with caplog.at_level(logging.DEBUG, logger="crab_api2.firmware"):
        fake.inject(frame("D", b"Bridge: Num 7\0"))
        crab.get_health("FreeRam")  # round trip so the D frame has been read
    assert (
        "crab_api2.firmware",
        logging.DEBUG,
        "Bridge: Num 7",
    ) in caplog.record_tuples


def test_nothing_printed(fake, capsys):
    with Crab(BRIDGE) as client:
        client.send(TicketType.ASYNCHRONOUS, [herkulex.test_react("Shoulder", 1)])
        client.get_health("FreeRam")
    assert capsys.readouterr().out == ""
