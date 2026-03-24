import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))


class ImportTests(unittest.TestCase):
    def test_thin_client_imports_without_serial_dependency(self) -> None:
        import crab_client

        self.assertTrue(hasattr(crab_client, "CrabClient"))
        self.assertTrue(hasattr(crab_client, "RosTransport"))


if __name__ == "__main__":
    unittest.main()
