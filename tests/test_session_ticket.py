import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from Coral_Multiplexor.session_manager import SessionManager
from Coral_Multiplexor.ticket_manager import TicketManager


class SessionTicketTests(unittest.TestCase):
    def test_session_creation(self) -> None:
        manager = SessionManager()
        session = manager.create("alpha")
        self.assertEqual(session.session_id, 1)
        self.assertTrue(manager.exists(1))

    def test_ticket_ownership(self) -> None:
        manager = TicketManager()
        manager.create(session_id=1, ticket_id=2)
        with self.assertRaises(PermissionError):
            manager.require_owner(9, 2)


if __name__ == "__main__":
    unittest.main()
