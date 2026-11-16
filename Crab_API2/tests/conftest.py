import pytest
from fakes import FakeFirmware

from crab_api2.link import Link


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
