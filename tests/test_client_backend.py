import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from crab_client import CrabClient, HealthMetric, HerkuleX, JogLedColor, TicketType
from crab_client.client import CrabClientError, InMemoryTransport
from Coral_Multiplexor import CoralMultiplexorBackend, DEFAULT_BRIDGE_PATH


class ClientBackendTests(unittest.TestCase):
    def setUp(self) -> None:
        self.backend = CoralMultiplexorBackend(str(DEFAULT_BRIDGE_PATH))
        self.client = CrabClient(InMemoryTransport(self.backend))

    def test_requires_connect(self) -> None:
        with self.assertRaises(CrabClientError):
            self.client.get_bridge_status()

    def test_submit_ticket_flow(self) -> None:
        self.client.connect("test-client")
        command = HerkuleX.move_one("Sholder", 10.0, 40, JogLedColor.LED_GREEN)
        ticket_id = self.client.submit_ticket(TicketType.ASYNCHRONOUS, [command])
        state = self.client.get_ticket_info(ticket_id)
        self.assertEqual(state.session_id, self.client.session_id)
        self.assertEqual(state.state, "queued")
        self.assertEqual(self.client.get_health(HealthMetric.CPU), "simulated:CPU")


if __name__ == "__main__":
    unittest.main()
