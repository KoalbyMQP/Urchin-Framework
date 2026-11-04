"""
Tests for the DBOXpy Python bindings (Clam/D-BOX/src/DBOX_bindings.cpp).

Run from the folder that contains the built DBOXpy module:
    python -m unittest test_DBOXpy        (or: python -m pytest test_DBOXpy.py)

Uses fake serial ports everywhere, plus a pseudo-terminal (a real kernel
serial-style device) for the Linux port tests, so no hardware is needed.
"""

import gc
import os
import select
import sys
import threading
import time
import unittest

import DBOXpy as d


def frame_bytes(vpid, stream, text):
    return bytes(d.DBOXraw(d.DBOX(vpid, stream, text)))


def fake_pair():
    link = d.FakeSerialLink("A", "B")
    a = d.FakeSerialPort(link, d.FakeSeralHost.Side.A)
    b = d.FakeSerialPort(link, d.FakeSeralHost.Side.B)
    assert a.openraw("A", 115200, 10) and b.openraw("B", 115200, 10)
    return d.DBOXprotocol(a, "/dev/null"), d.DBOXprotocol(b, "/dev/null"), a, b


class PacketTypes(unittest.TestCase):
    def test_packet_fields(self):
        p = d.DBOX(0, "N", "Spin 200")
        self.assertEqual(p.VPID, 0)
        self.assertEqual(p.stream, "N")
        self.assertEqual(p.Stream, ord("N"))
        self.assertEqual(p.text, "Spin 200")
        self.assertEqual(p.payload, b"Spin 200")
        self.assertEqual(p, d.DBOX(0, ord("N"), "Spin 200"))

    def test_wire_format_matches_the_spec(self):
        # 07 | ID 00 | 'N' 4E | len 08 00 | CRC 6A 4A | "Spin 200"
        self.assertEqual(frame_bytes(0, "N", "Spin 200"),
                         bytes.fromhex("07004E08006A4A") + b"Spin 200")


class ProtocolOverFakePorts(unittest.TestCase):
    def test_send_and_receive(self):
        pa, pb, _, _ = fake_pair()
        self.assertTrue(pa.send(d.DBOX(3, "M", "move 90")))
        got = d.DBOX()
        self.assertTrue(pb.receive(got, 1000))
        self.assertEqual(got, d.DBOX(3, "M", "move 90"))

    def test_receive_times_out(self):
        pa, _, _, _ = fake_pair()
        t0 = time.monotonic()
        self.assertFalse(pa.receive(d.DBOX(), 150))
        self.assertLess(time.monotonic() - t0, 1.5)

    def test_receive_skips_junk_and_false_start(self):
        pa, _, _, b = fake_pair()
        b.write(b"boot\x07junk!!" + frame_bytes(4, "S", "sensor ok"))
        got = d.DBOX()
        self.assertTrue(pa.receive(got, 1000))
        self.assertEqual(got.text, "sensor ok")

    def test_objects_stay_alive_while_used(self):
        # Only the protocol is kept; the port/link it uses must not be freed.
        def make():
            link = d.FakeSerialLink("A", "B")
            a = d.FakeSerialPort(link, d.FakeSeralHost.Side.A)
            a.openraw("A", 115200, 10)
            return d.DBOXprotocol(a, "/dev/null")
        proto = make()
        gc.collect()
        self.assertFalse(proto.receive(d.DBOX(), 50))   # would crash if freed


class DiscoveryOverFakePorts(unittest.TestCase):
    def test_finds_the_device_port(self):
        to_mouse = d.FakeSerialLink("COM3", "Mouse")
        to_esp = d.FakeSerialLink("COM5", "UART0")
        pc_host, esp_host = d.FakeSeralHost(), d.FakeSeralHost()
        pc_host.add_plug(to_mouse, d.FakeSeralHost.Side.A)
        pc_host.add_plug(to_esp, d.FakeSeralHost.Side.A)
        esp_host.add_plug(to_esp, d.FakeSeralHost.Side.B)

        pc_cfg = d.DconfigUart("Urchin", "", 115200, 10)
        esp_cfg = d.DconfigUart("Urchin", "UART0", 115200, 10)
        pc_disco = d.UartDiscovery(True, pc_cfg)
        esp_disco = d.UartDiscovery(False, esp_cfg)
        pc_port = d.FakeSerialPort(pc_disco, pc_host)
        esp_port = d.FakeSerialPort(esp_disco, esp_host)
        pc_proto = d.DBOXprotocol(pc_port, "/dev/null")
        esp_proto = d.DBOXprotocol(esp_port, "/dev/null")
        pc_disco.handshake_timeout_ms = 200

        self.assertEqual(pc_port.find(), ["COM3", "COM5"])

        # Passive side in a Python thread: only works if discover() releases the GIL.
        result = {}
        t = threading.Thread(target=lambda: result.setdefault("esp", esp_disco.discover()))
        t.start()
        self.assertEqual(pc_disco.discover(), 1)
        t.join(5)
        self.assertEqual(result.get("esp"), 1)
        self.assertEqual(pc_cfg.port, "COM5")

        self.assertTrue(pc_proto.send(d.DBOX(0, "N", "Info")))
        got = d.DBOX()
        self.assertTrue(esp_proto.receive(got, 1000))
        self.assertEqual(got.text, "Info")


@unittest.skipUnless(hasattr(d, "LinuxSerialPort") and sys.platform.startswith("linux"), "Linux only")
class LinuxPortOverPty(unittest.TestCase):
    def setUp(self):
        self.master, self.slave = os.openpty()
        self.name = os.ttyname(self.slave)

    def tearDown(self):
        os.close(self.slave)
        os.close(self.master)

    def read_master(self, seconds):
        out, end = b"", time.monotonic() + seconds
        while time.monotonic() < end:
            if select.select([self.master], [], [], 0.02)[0]:
                out += os.read(self.master, 1024)
        return out

    def test_send_and_receive_on_a_real_device(self):
        port = d.LinuxSerialPort()
        self.assertTrue(port.openraw(self.name, 115200, 10))
        proto = d.DBOXprotocol(port, "/dev/null")

        self.assertTrue(proto.send(d.DBOX(0, "N", "Spin 200")))
        self.assertEqual(self.read_master(0.2), frame_bytes(0, "N", "Spin 200"))

        os.write(self.master, b"noise\x07xx" + frame_bytes(0, "E", "OK SPIN 200"))
        got = d.DBOX()
        self.assertTrue(proto.receive(got, 1000))
        self.assertEqual((got.stream, got.text), ("E", "OK SPIN 200"))

    def test_discovery_handshake_on_a_real_device(self):
        hello = frame_bytes(0, "S", "Urchin")
        stop = threading.Event()

        def device():               # behaves like DBoxMotor: answer the name
            seen = b""
            while not stop.is_set():
                if select.select([self.master], [], [], 0.02)[0]:
                    seen += os.read(self.master, 1024)
                    if hello in seen:
                        os.write(self.master, hello)
                        seen = b""
        t = threading.Thread(target=device)
        t.start()
        try:
            cfg = d.DconfigUart("Urchin", "", 115200, 10)
            disco = d.UartDiscovery(True, cfg)
            port = d.LinuxSerialPort(disco)
            d.DBOXprotocol(port, "/dev/null")
            disco.handshake_timeout_ms = 300
            self.assertTrue(disco.active_discover_single(self.name, 3))
        finally:
            stop.set()
            t.join()


if __name__ == "__main__":
    unittest.main()
