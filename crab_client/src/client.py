from __future__ import annotations

from collections.abc import Callable
from typing import Any, Protocol

from .events import BridgeStatus, DebugEvent, ReactionEvent, TicketState
from .types import CommandItem, HealthMetric, TicketType


class CrabClientError(RuntimeError):
    pass


class ClientTransport(Protocol):
    def register_client(self, client_name: str) -> int: ...
    def unregister_client(self, session_id: int) -> None: ...
    def get_bridge_status(self, session_id: int) -> BridgeStatus: ...
    def submit_ticket(self, session_id: int, ticket_type: TicketType, items: list[CommandItem], chained: bool) -> int: ...
    def close_ticket(self, session_id: int, ticket_id: int) -> None: ...
    def get_ticket_info(self, session_id: int, ticket_id: int) -> TicketState: ...
    def get_health(self, session_id: int, metric: HealthMetric) -> str: ...
    def subscribe_debug(self, session_id: int, handler: Callable[[DebugEvent], None]) -> None: ...
    def subscribe_ticket_state(self, session_id: int, handler: Callable[[TicketState], None]) -> None: ...
    def subscribe_reactions(self, session_id: int, handler: Callable[[ReactionEvent], None]) -> None: ...


class CrabClient:
    def __init__(self, transport: ClientTransport) -> None:
        self._transport = transport
        self._session_id: int | None = None

    @property
    def session_id(self) -> int:
        if self._session_id is None:
            raise CrabClientError("Client is not connected")
        return self._session_id

    def connect(self, client_name: str) -> int:
        self._session_id = self._transport.register_client(client_name)
        return self._session_id

    def close(self) -> None:
        if self._session_id is None:
            return
        self._transport.unregister_client(self._session_id)
        close_method = getattr(self._transport, "close", None)
        if callable(close_method):
            close_method()
        self._session_id = None

    def get_bridge_status(self) -> BridgeStatus:
        return self._transport.get_bridge_status(self.session_id)

    def submit_ticket(self, ticket_type: TicketType, items: list[CommandItem], chained: bool = False) -> int:
        return self._transport.submit_ticket(self.session_id, ticket_type, items, chained)

    def close_ticket(self, ticket_id: int) -> None:
        self._transport.close_ticket(self.session_id, ticket_id)

    def get_ticket_info(self, ticket_id: int) -> TicketState:
        return self._transport.get_ticket_info(self.session_id, ticket_id)

    def get_health(self, metric: HealthMetric) -> str:
        return self._transport.get_health(self.session_id, metric)

    def subscribe_debug(self, handler: Callable[[DebugEvent], None]) -> None:
        self._transport.subscribe_debug(self.session_id, handler)

    def subscribe_ticket_state(self, handler: Callable[[TicketState], None]) -> None:
        self._transport.subscribe_ticket_state(self.session_id, handler)

    def subscribe_reactions(self, handler: Callable[[ReactionEvent], None]) -> None:
        self._transport.subscribe_reactions(self.session_id, handler)


class InMemoryTransport:
    def __init__(self, backend: Any) -> None:
        self._backend = backend

    def register_client(self, client_name: str) -> int:
        return self._backend.register_client(client_name)

    def unregister_client(self, session_id: int) -> None:
        self._backend.unregister_client(session_id)

    def get_bridge_status(self, session_id: int) -> BridgeStatus:
        return self._backend.get_bridge_status(session_id)

    def submit_ticket(self, session_id: int, ticket_type: TicketType, items: list[CommandItem], chained: bool) -> int:
        return self._backend.submit_ticket(session_id, ticket_type, items, chained)

    def close_ticket(self, session_id: int, ticket_id: int) -> None:
        self._backend.close_ticket(session_id, ticket_id)

    def get_ticket_info(self, session_id: int, ticket_id: int) -> TicketState:
        return self._backend.get_ticket_info(session_id, ticket_id)

    def get_health(self, session_id: int, metric: HealthMetric) -> str:
        return self._backend.get_health(session_id, metric)

    def subscribe_debug(self, session_id: int, handler: Callable[[DebugEvent], None]) -> None:
        self._backend.subscribe_debug(session_id, handler)

    def subscribe_ticket_state(self, session_id: int, handler: Callable[[TicketState], None]) -> None:
        self._backend.subscribe_ticket_state(session_id, handler)

    def subscribe_reactions(self, session_id: int, handler: Callable[[ReactionEvent], None]) -> None:
        self._backend.subscribe_reactions(session_id, handler)
