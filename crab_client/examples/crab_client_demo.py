from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))

from crab_client import CrabClient, HealthMetric, HerkuleX, InMemoryTransport, JogLedColor, TicketType
from Coral_Multiplexor import CoralMultiplexorBackend, DEFAULT_BRIDGE_PATH


def main() -> None:
    backend = CoralMultiplexorBackend(str(DEFAULT_BRIDGE_PATH))
    client = CrabClient(InMemoryTransport(backend))
    session_id = client.connect("demo-client")
    print(f"connected session={session_id}")
    print(client.get_bridge_status())

    command = HerkuleX.move_one("Sholder", -23.0, 40, JogLedColor.LED_BLUE)
    ticket_id = client.submit_ticket(TicketType.ASYNCHRONOUS, [command], chained=False)
    print(f"ticket={ticket_id}")
    print(client.get_ticket_info(ticket_id))
    print(client.get_health(HealthMetric.CPU))
    client.close()


if __name__ == "__main__":
    main()
