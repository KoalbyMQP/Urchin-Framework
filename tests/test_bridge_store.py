import struct
import unittest
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from Coral_Multiplexor import DEFAULT_BRIDGE_PATH
from Coral_Multiplexor.bridge_store import BridgeStore


class BridgeStoreTests(unittest.TestCase):
    def test_load_bridge_file(self) -> None:
        store = BridgeStore.load(str(DEFAULT_BRIDGE_PATH))
        self.assertEqual(store.version, "0.1")
        self.assertEqual(len(store.motors), 2)
        self.assertEqual(store.get_motor("Sholder").number, 7)

    def test_encode_bridge_packets(self) -> None:
        store = BridgeStore.load(str(DEFAULT_BRIDGE_PATH))
        packets = store.encode_bridge_packets()
        self.assertEqual(len(packets), 2)
        self.assertTrue(packets[0].startswith(b"BridgeAdd"))
        packed = packets[0][len(b"BridgeAdd") :]
        last = struct.unpack("<B30s10s50siii?", packed)[-1]
        self.assertFalse(last)
        last = struct.unpack("<B30s10s50siii?", packets[1][len(b"BridgeAdd") :])[-1]
        self.assertTrue(last)


if __name__ == "__main__":
    unittest.main()
