import functools
import logging
from pathlib import Path
from types import SimpleNamespace

import pytest

from crab_api2 import cli, link
from crab_api2.crab import Crab

BRIDGE = Path(__file__).parent / "fixtures" / "bridge.toml"  # Shoulder = 7, Hip = 8


@pytest.fixture(autouse=True)
def restore_logging():
    root = logging.getLogger()
    saved = root.handlers[:], root.level
    yield
    root.handlers[:], level = saved
    root.setLevel(level)


def port_info(device, description="n/a", serial_number=None, vid=None, pid=None):
    return SimpleNamespace(
        device=device,
        description=description,
        serial_number=serial_number,
        vid=vid,
        pid=pid,
    )


# --- ports ---


def test_ports_lists_and_probes_in_priority_order(monkeypatch, capsys):
    ports = [
        port_info("/dev/ttyS0"),
        port_info("/dev/ttyUSB0", "CP2102 USB to UART", "urchin", 0x10C4, 0xEA60),
    ]
    probed = []

    def fake_probe(device):
        probed.append(device)
        return device == "/dev/ttyUSB0"

    monkeypatch.delenv("URCHIN_PORT", raising=False)
    monkeypatch.setattr(link, "comports", lambda: ports)
    monkeypatch.setattr(link, "probe", fake_probe)

    assert cli.main(["ports"]) == 0
    out = capsys.readouterr().out.splitlines()
    assert probed == ["/dev/ttyUSB0", "/dev/ttyS0"]
    assert out[0].startswith("/dev/ttyUSB0")
    assert "CP2102 USB to UART" in out[0]
    assert "10C4:EA60" in out[0] and "sn=urchin" in out[0]
    assert out[0].endswith("Urchin board")
    assert out[1].startswith("/dev/ttyS0") and out[1].endswith("no reply")


def test_ports_none_found(monkeypatch, capsys):
    monkeypatch.delenv("URCHIN_PORT", raising=False)
    monkeypatch.setattr(link, "comports", list)
    assert cli.main(["ports"]) == 0
    assert capsys.readouterr().out == "No serial ports found.\n"


def test_ports_mentions_urchin_port(monkeypatch, capsys):
    monkeypatch.setenv("URCHIN_PORT", "socket://127.0.0.1:5555")
    monkeypatch.setattr(link, "comports", list)
    cli.main(["ports"])
    assert "URCHIN_PORT is set: socket://127.0.0.1:5555" in capsys.readouterr().out


# --- check ---


def test_check_all_responding(fake, capsys):
    fake.ack_lines = [
        b"GetHealth:AckCheck:Checking 7",
        b"HF7B",
        b"GetHealth:AckCheck:Checking 8",
        b"HF8B",
    ]
    assert cli.main(["check", str(BRIDGE), "--url", "socket://127.0.0.1:5555"]) == 0
    out = capsys.readouterr().out
    assert fake.opened_with == ["socket://127.0.0.1:5555"]
    assert "Bridge uploaded: 2 motors" in out
    assert "FreeRam: 123456 bytes" in out
    assert "Shoulder (motor 7): ok, detail=0x42 error=0x00" in out
    assert "Hip (motor 8): ok, detail=0x42 error=0x00" in out


def test_check_motor_not_responding(fake, capsys):
    fake.ack_lines = [
        b"GetHealth:AckCheck:Checking 7",
        b"HF7B",
        b"GetHealth:AckCheck:Checking 8",
        b"HM800",
    ]
    assert cli.main(["check", str(BRIDGE)]) == 1
    assert "Hip (motor 8): NOT RESPONDING" in capsys.readouterr().out


def test_check_motor_missing_from_reply(fake, capsys):
    fake.ack_lines = [b"GetHealth:AckCheck:Checking 7", b"HF7B"]
    assert cli.main(["check", str(BRIDGE)]) == 1
    assert "Hip (motor 8): NO REPORT" in capsys.readouterr().out


def test_check_bad_bridge(tmp_path, fake, capsys):
    bad = tmp_path / "bad.toml"
    bad.write_text("[meta]\n")
    assert cli.main(["check", str(bad)]) == 1
    assert "error:" in capsys.readouterr().err


def test_check_no_reply(fake, monkeypatch, capsys):
    fake.silent = True
    monkeypatch.setattr(cli, "Crab", functools.partial(Crab, reply_timeout=0.2))
    assert cli.main(["check", str(BRIDGE)]) == 1
    assert "error: no reply to FreeRam" in capsys.readouterr().err


def test_check_no_board(monkeypatch, capsys):
    monkeypatch.delenv("URCHIN_PORT", raising=False)
    monkeypatch.setattr(link, "comports", list)
    assert cli.main(["check", str(BRIDGE)]) == 1
    assert "error: No Urchin board found" in capsys.readouterr().err


# --- options ---


def test_verbose_enables_debug_logs(fake, capsys):
    fake.ack_lines = [
        b"GetHealth:AckCheck:Checking 7",
        b"HF7B",
        b"GetHealth:AckCheck:Checking 8",
        b"HF8B",
    ]
    cli.main(["-v", "check", str(BRIDGE)])
    assert "[DEBUG]" in capsys.readouterr().err


def test_requires_subcommand(capsys):
    with pytest.raises(SystemExit) as exc_info:
        cli.main([])
    assert exc_info.value.code == 2


def test_no_demo_command(capsys):
    with pytest.raises(SystemExit):
        cli.main(["demo"])


def test_check_unopenable_port(capsys):
    assert cli.main(["check", str(BRIDGE), "--url", "/dev/definitely-not-a-port"]) == 1
    assert "error: Could not open /dev/definitely-not-a-port" in capsys.readouterr().err
