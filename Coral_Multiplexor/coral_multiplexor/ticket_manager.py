from __future__ import annotations

from dataclasses import replace

from crab_client.events import TicketState


class TicketManager:
    def __init__(self) -> None:
        self._tickets: dict[int, TicketState] = {}

    def create(self, session_id: int, ticket_id: int, state: str = "accepted", firmware_error: int = 0, detail: str = "") -> TicketState:
        ticket = TicketState(
            ticket_id=ticket_id,
            session_id=session_id,
            state=state,
            firmware_error=firmware_error,
            detail=detail,
        )
        self._tickets[ticket_id] = ticket
        return ticket

    def get(self, ticket_id: int) -> TicketState:
        ticket = self._tickets.get(ticket_id)
        if ticket is None:
            raise KeyError(f"Unknown ticket_id {ticket_id}")
        return ticket

    def require_owner(self, session_id: int, ticket_id: int) -> TicketState:
        ticket = self.get(ticket_id)
        if ticket.session_id != session_id:
            raise PermissionError(f"Session {session_id} does not own ticket {ticket_id}")
        return ticket

    def update(self, ticket_id: int, **changes: object) -> TicketState:
        ticket = self.get(ticket_id)
        ticket = replace(ticket, **changes)
        self._tickets[ticket_id] = ticket
        return ticket
