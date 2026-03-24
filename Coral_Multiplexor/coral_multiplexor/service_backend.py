from __future__ import annotations

from collections import defaultdict
from collections.abc import Callable
from typing import Any

from crab_client.events import BridgeStatus, DebugEvent, ReactionEvent, TicketState
from crab_client.types import CommandItem, HealthMetric, TicketType

from .bridge_store import BridgeStore, BridgeValidationError
from .firmware_protocol import FirmwareProtocol
from .session_manager import SessionManager
from .ticket_manager import TicketManager


class CoralMultiplexorBackend:
    """
    Runtime-agnostic backend that can be hosted by a ROS2 node or tested directly.
    """

    def __init__(self, bridge_path: str, transport: Any | None = None, debug: bool = False) -> None:
        self._bridge_path = bridge_path
        self._transport = transport
        self._debug = debug
        self._protocol = FirmwareProtocol()
        self._sessions = SessionManager()
        self._tickets = TicketManager()
        self._bridge: BridgeStore | None = None
        self._ready = False
        self._status_message = "Not initialized"
        self._debug_handlers: dict[int, list[Callable[[DebugEvent], None]]] = defaultdict(list)
        self._reaction_handlers: dict[int, list[Callable[[ReactionEvent], None]]] = defaultdict(list)
        self._ticket_handlers: dict[int, list[Callable[[TicketState], None]]] = defaultdict(list)
        self._initialize()

    def _initialize(self) -> None:
        try:
            self._bridge = BridgeStore.load(self._bridge_path)
            if self._transport is not None:
                for packet in self._bridge.encode_bridge_packets():
                    self._transport.send_packet(0, b"N", packet)
            self._ready = True
            self._status_message = "ready"
        except (BridgeValidationError, OSError, ConnectionError) as exc:
            self._ready = False
            self._status_message = str(exc)

    def register_client(self, client_name: str) -> int:
        return self._sessions.create(client_name).session_id

    def unregister_client(self, session_id: int) -> None:
        self._sessions.remove(session_id)

    def get_bridge_status(self, session_id: int) -> BridgeStatus:
        self._sessions.require(session_id)
        bridge = self._bridge
        return BridgeStatus(
            loaded=bridge is not None and self._ready,
            bridge_version=bridge.version if bridge is not None else "",
            motor_count=len(bridge.motors) if bridge is not None else 0,
            source_path=bridge.source_path if bridge is not None else self._bridge_path,
            message=self._status_message,
        )

    def submit_ticket(self, session_id: int, ticket_type: TicketType, items: list[CommandItem], chained: bool) -> int:
        self._sessions.require(session_id)
        self._require_ready()
        if not items:
            raise ValueError("submit_ticket requires at least one item")
        assert self._bridge is not None
        ticket_id = self._issue_ticket(ticket_type, items)
        state = self._tickets.create(session_id, ticket_id, state="queued", detail="Ticket accepted by multiplexor")
        self._emit_ticket_state(state)
        return ticket_id

    def _issue_ticket(self, ticket_type: TicketType, items: list[CommandItem]) -> int:
        if self._transport is None:
            ticket_id = len(getattr(self, "_local_ticket_ids", [])) + 1
            self.__dict__.setdefault("_local_ticket_ids", []).append(ticket_id)
            return ticket_id

        self._transport.send_packet(0, b"N", self._protocol.encode_format_ticket(ticket_type))
        error_packet = self._transport.read_packet()
        if error_packet is None:
            raise RuntimeError("No response received for FormatTicket")
        firmware_error = self._protocol.parse_exchange_int(error_packet["data"])
        if firmware_error != 0:
            raise RuntimeError(f"Firmware refused ticket: error {firmware_error}")
        ticket_packet = self._transport.read_packet()
        if ticket_packet is None:
            raise RuntimeError("No ticket id received from firmware")
        ticket_id = self._protocol.parse_exchange_uint(ticket_packet["data"])

        for item in items:
            self._bridge.get_motor(item.joint)
            self._transport.send_packet(0, b"N", self._protocol.encode_load_ticket(ticket_id, item))
        self._transport.send_packet(0, b"N", self._protocol.encode_punch_ticket(ticket_id))
        return ticket_id

    def close_ticket(self, session_id: int, ticket_id: int) -> None:
        self._sessions.require(session_id)
        state = self._tickets.require_owner(session_id, ticket_id)
        if self._transport is not None:
            self._transport.send_packet(0, b"N", self._protocol.encode_close_ticket(ticket_id))
        state = self._tickets.update(ticket_id, state="closed", detail="Closed by client")
        self._emit_ticket_state(state)

    def get_ticket_info(self, session_id: int, ticket_id: int) -> TicketState:
        self._sessions.require(session_id)
        return self._tickets.require_owner(session_id, ticket_id)

    def get_health(self, session_id: int, metric: HealthMetric) -> str:
        self._sessions.require(session_id)
        self._require_ready()
        if self._transport is None:
            return f"simulated:{metric.value}"
        self._transport.send_packet(0, b"N", self._protocol.encode_get_health(metric.value))
        packet = self._transport.read_packet()
        if packet is None:
            raise RuntimeError("No health response received")
        return packet["data"].decode("utf-8", errors="ignore").rstrip("\x00")

    def subscribe_debug(self, session_id: int, handler: Callable[[DebugEvent], None]) -> None:
        self._sessions.require(session_id)
        self._debug_handlers[session_id].append(handler)

    def subscribe_ticket_state(self, session_id: int, handler: Callable[[TicketState], None]) -> None:
        self._sessions.require(session_id)
        self._ticket_handlers[session_id].append(handler)

    def subscribe_reactions(self, session_id: int, handler: Callable[[ReactionEvent], None]) -> None:
        self._sessions.require(session_id)
        self._reaction_handlers[session_id].append(handler)

    def process_firmware_packet(self, packet: dict[str, Any]) -> None:
        stream = packet["Stream"]
        payload = packet["data"]
        if stream == b"D":
            event = DebugEvent(session_id=0, ticket_id=0, text=payload.decode("utf-8", errors="ignore").rstrip("\x00"))
            for handlers in self._debug_handlers.values():
                for handler in handlers:
                    handler(event)
            return
        if stream == b"R":
            event = ReactionEvent(session_id=0, ticket_id=0, code=self._protocol.parse_exchange_int(payload), detail="")
            for handlers in self._reaction_handlers.values():
                for handler in handlers:
                    handler(event)

    def _emit_ticket_state(self, state: TicketState) -> None:
        for handler in self._ticket_handlers[state.session_id]:
            handler(state)

    def _require_ready(self) -> None:
        if not self._ready:
            raise RuntimeError(self._status_message)
