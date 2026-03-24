from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class BridgeStatus:
    loaded: bool
    bridge_version: str
    motor_count: int
    source_path: str
    message: str = ""


@dataclass(frozen=True)
class TicketState:
    ticket_id: int
    session_id: int
    state: str
    firmware_error: int = 0
    detail: str = ""


@dataclass(frozen=True)
class DebugEvent:
    session_id: int
    ticket_id: int
    text: str


@dataclass(frozen=True)
class ReactionEvent:
    session_id: int
    ticket_id: int
    code: int
    detail: str = ""


@dataclass(frozen=True)
class ExchangeEvent:
    session_id: int
    ticket_id: int
    payload: bytes
