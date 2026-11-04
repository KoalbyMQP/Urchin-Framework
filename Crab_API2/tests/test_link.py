import logging
from collections.abc import Callable
from types import SimpleNamespace

import pytest

from crab_api2 import link
from crab_api2.link import (
    DATA_SIZE,
    FRAME_SIZE,
    START_MARKER,
    Frame,
    Link,
    LinkError,
    decode_frame,
    encode_frame,
    find_urchin_port,
    handshake,
    order_ports,
)


class FakePort:
    """In-memory stand-in for a pyserial port.

    ``responder`` is called with every write and returns bytes to queue for reading.
    """

    def __init__(
        self, incoming: bytes = b"", responder: Callable[[bytes], bytes] | None = None
    ):
        self.buffer = bytearray(incoming)
        self.written: list[bytes] = []
        self.responder = responder
        self.closed = False

    def write(self, data: bytes) -> int:
        self.written.append(bytes(data))
        if self.responder:
            self.buffer += self.responder(bytes(data))
        return len(data)

    def read(self, size: int = 1) -> bytes:
        chunk = bytes(self.buffer[:size])
        del self.buffer[:size]
        return chunk

    def read_until(self, expected: bytes = b"\n", size: int | None = None) -> bytes:
        index = self.buffer.find(expected)
        end = len(self.buffer) if index == -1 else index + len(expected)
        return self.read(end)

    def reset_input_buffer(self) -> None:
        self.buffer.clear()

    def close(self) -> None:
        self.closed = True


def firmware_frame(stream: str, data: bytes, vpid: int = 0) -> bytes:
    """A frame as the firmware sends it (Shipping.c::SendMessage)."""
    return START_MARKER + bytes([vpid]) + stream.encode() + data.ljust(DATA_SIZE, b"\0")


# --- framing ---


def test_frame_constants():
    assert START_MARKER == b"\x07"
    assert DATA_SIZE == 1024
    assert FRAME_SIZE == 1026


def test_encode_frame_layout():
    assert encode_frame(5, b"Validate") == (
        b"\x07" + b"\x05" + b"N" + b"Validate" + b"\0" * (1024 - len(b"Validate"))
    )


def test_encode_frame_full_payload():
    payload = bytes(range(256)) * 4
    assert encode_frame(0, payload) == b"\x07\x00N" + payload


@pytest.mark.parametrize("vpid", [-1, 256])
def test_encode_frame_bad_vpid(vpid):
    with pytest.raises(LinkError, match="VPID must be between 0 and 255"):
        encode_frame(vpid, b"")


def test_encode_frame_payload_too_large():
    with pytest.raises(LinkError, match="payload is 1025 bytes; max is 1024"):
        encode_frame(0, b"x" * 1025)


def test_decode_frame_keeps_raw_data():
    data = b"\x01\x00\x00\x00" + b"\0" * 1020
    assert decode_frame(b"\x03E" + data) == Frame(vpid=3, stream="E", data=data)


@pytest.mark.parametrize("size", [0, 1025, 1027])
def test_decode_frame_wrong_size(size):
    with pytest.raises(LinkError, match=f"expected 1026 bytes, got {size}"):
        decode_frame(b"\0" * size)


# --- Link send/read ---


def test_send_writes_encoded_frame():
    port = FakePort()
    Link(port).send(0, b"Validate")
    assert port.written == [encode_frame(0, b"Validate")]


def test_send_rejects_oversized_payload():
    port = FakePort()
    with pytest.raises(LinkError):
        Link(port).send(0, b"x" * 1025)
    assert port.written == []


def test_read_frame():
    port = FakePort(firmware_frame("D", b"URCHIN"))
    frame = Link(port).read()
    assert frame == Frame(0, "D", b"URCHIN".ljust(1024, b"\0"))


def test_read_skips_garbage_before_marker():
    port = FakePort(b"boot log junk\r\n" + firmware_frame("E", b"\x01"))
    assert Link(port).read().stream == "E"


def test_read_consecutive_frames():
    port = FakePort(firmware_frame("E", b"a") + firmware_frame("R", b"b", vpid=2))
    lnk = Link(port)
    assert [(f.vpid, f.stream, f.data[:1]) for f in (lnk.read(), lnk.read())] == [
        (0, "E", b"a"),
        (2, "R", b"b"),
    ]


def test_read_data_may_contain_marker_byte():
    data = b"\x07\x07\x07"
    port = FakePort(firmware_frame("E", data))
    assert Link(port).read().data[:3] == data


def test_read_timeout_returns_none():
    assert Link(FakePort()).read() is None


def test_read_timeout_without_marker_returns_none():
    assert Link(FakePort(b"no marker here")).read() is None


def test_read_short_frame_returns_none_and_warns(caplog):
    port = FakePort(START_MARKER + b"\x00D" + b"partial")
    with caplog.at_level(logging.WARNING, logger="crab_api2.link"):
        assert Link(port).read() is None
    assert "Incomplete frame" in caplog.text


def test_context_manager_closes_port():
    port = FakePort()
    with Link(port):
        pass
    assert port.closed


# --- opening ---


def test_open_explicit_url(monkeypatch):
    monkeypatch.delenv("URCHIN_PORT", raising=False)
    monkeypatch.setattr(link, "find_urchin_port", _fail_discovery)
    with Link.open("loop://") as lnk:
        lnk.send(1, b"echo")
        frame = lnk.read()
    assert frame == Frame(1, "N", b"echo".ljust(1024, b"\0"))


def test_open_uses_env_over_discovery(monkeypatch):
    monkeypatch.setenv("URCHIN_PORT", "loop://")
    monkeypatch.setattr(link, "find_urchin_port", _fail_discovery)
    with Link.open() as lnk:
        assert lnk.port.port == "loop://"


def test_open_explicit_url_over_env(monkeypatch):
    monkeypatch.setenv("URCHIN_PORT", "socket://127.0.0.1:1")
    with Link.open("loop://") as lnk:
        assert lnk.port.port == "loop://"


def test_open_falls_back_to_discovery(monkeypatch):
    monkeypatch.delenv("URCHIN_PORT", raising=False)
    monkeypatch.setattr(link, "find_urchin_port", lambda: "loop://")
    with Link.open() as lnk:
        assert lnk.port.port == "loop://"


def _fail_discovery() -> str:
    raise AssertionError("discovery should not run")


# --- discovery ---


def port_info(device, serial_number=None, vid=None, pid=None):
    return SimpleNamespace(device=device, serial_number=serial_number, vid=vid, pid=pid)


def test_order_ports():
    ports = [
        port_info("/dev/ttyS0"),
        port_info("/dev/ttyUSB1", vid=0x10C4, pid=0xEA60),
        port_info("/dev/ttyACM0", vid=0x2341, pid=0x0043),  # Arduino, unknown
        port_info("/dev/ttyUSB0", serial_number="Urchin", vid=0x10C4, pid=0xEA60),
        port_info("/dev/ttyACM1", vid=0x303A, pid=0x1001),
    ]
    assert order_ports(ports) == [
        "/dev/ttyUSB0",  # serial number match
        "/dev/ttyUSB1",  # known ESP32 USB-UART chips, original order kept
        "/dev/ttyACM1",
        "/dev/ttyS0",  # everything else
        "/dev/ttyACM0",
    ]


def test_handshake_success():
    port = FakePort(responder=lambda _: firmware_frame("D", b"URCHIN"))
    assert handshake(port, attempts=3, reply_timeout=0.05)
    assert port.written == [encode_frame(0, b"Validate")]


def test_handshake_skips_other_frames():
    replies = firmware_frame("D", b"FormatTicket:...") + firmware_frame("D", b"URCHIN")
    port = FakePort(responder=lambda _: replies)
    assert handshake(port, attempts=1, reply_timeout=0.05)


def test_handshake_ignores_urchin_on_wrong_stream():
    port = FakePort(responder=lambda _: firmware_frame("E", b"URCHIN"))
    assert not handshake(port, attempts=1, reply_timeout=0.05)


def test_handshake_retries_then_fails():
    port = FakePort()
    assert not handshake(port, attempts=3, reply_timeout=0.01)
    assert len(port.written) == 3


def test_handshake_succeeds_on_retry():
    replies = iter([b"", firmware_frame("D", b"URCHIN")])
    port = FakePort(responder=lambda _: next(replies))
    assert handshake(port, attempts=3, reply_timeout=0.01)
    assert len(port.written) == 2


def test_find_urchin_port_stops_at_first_match(monkeypatch):
    ports = [port_info("/dev/a", serial_number="urchin"), port_info("/dev/b")]
    probed = []

    def fake_probe(device: str) -> bool:
        probed.append(device)
        return True

    monkeypatch.setattr(link, "comports", lambda: ports)
    monkeypatch.setattr(link, "probe", fake_probe)
    assert find_urchin_port() == "/dev/a"
    assert probed == ["/dev/a"]


def test_find_urchin_port_probes_in_priority_order(monkeypatch):
    ports = [port_info("/dev/other"), port_info("/dev/esp", vid=0x1A86, pid=0x7523)]
    probed = []

    def fake_probe(device: str) -> bool:
        probed.append(device)
        return device == "/dev/other"

    monkeypatch.setattr(link, "comports", lambda: ports)
    monkeypatch.setattr(link, "probe", fake_probe)
    assert find_urchin_port() == "/dev/other"
    assert probed == ["/dev/esp", "/dev/other"]


def test_find_urchin_port_none_found(monkeypatch):
    monkeypatch.setattr(link, "comports", lambda: [port_info("/dev/ttyS0")])
    monkeypatch.setattr(link, "probe", lambda device: False)
    with pytest.raises(LinkError, match="No Urchin board found"):
        find_urchin_port()


def test_probe_handles_unopenable_port():
    assert not link.probe("/dev/definitely-not-a-port", settle=0)


def test_encode_matches_legacy_espserial_format():
    """Same bytes the original Crab_API ESPSerial.send_packet produced."""
    import struct

    payload = b"FormatTicketA"
    legacy = b"\x07" + struct.pack("<Bc1024s", 0, b"N", payload.ljust(1024, b"\x00"))
    assert encode_frame(0, payload) == legacy
