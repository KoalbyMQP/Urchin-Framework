import struct
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from crab_client.types import CommandItem, TicketType
from Coral_Multiplexor.firmware_protocol import FirmwareProtocol


class FirmwareProtocolTests(unittest.TestCase):
    def test_encode_format_ticket(self) -> None:
        self.assertEqual(FirmwareProtocol.encode_format_ticket(TicketType.ASYNCHRONOUS), b"FormatTicketA")

    def test_encode_load_ticket(self) -> None:
        item = CommandItem.from_primitives("Sholder", "MoveOne", [-23.0, 40, True])
        payload = FirmwareProtocol.encode_load_ticket(17, item)
        self.assertTrue(payload.startswith(b"LoadTicket"))
        ticket = struct.unpack("<I", payload[10:14])[0]
        self.assertEqual(ticket, 17)


if __name__ == "__main__":
    unittest.main()
