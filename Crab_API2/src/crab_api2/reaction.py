"""Reactions: asynchronous events the firmware sends back about a ticket.

The firmware sends reactions on the ``R`` stream
(``Urchin_Firmware/src/ESP_PI_Communication/Reaction.cpp::SendReaction``)::

    u32 ticket | u32 code | char joint[50] | u8 command_len | u8 values_len
    command | values_len x (char tag, 4-byte union)

Tags are ``i`` (int32), ``f`` (float32), and ``b`` (byte; only the first union byte is
set). A ``Reaction`` subscribes a callback to events for one joint and command; a
``ReactionRegistry`` binds reactions to tickets and dispatches decoded events.
"""

from __future__ import annotations

import logging
import struct
import threading
from collections.abc import Callable, Collection, Iterable
from dataclasses import dataclass

from crab_api2.bridge import MAX_JOINT

logger = logging.getLogger(__name__)

_HEADER = struct.Struct(f"<II{MAX_JOINT}sBB")
_VALUE_SIZE = 5
_DECODERS: dict[int, Callable[[bytes], int | float]] = {
    ord("i"): lambda raw: struct.unpack_from("<i", raw)[0],
    ord("f"): lambda raw: struct.unpack_from("<f", raw)[0],
    ord("b"): lambda raw: struct.unpack_from("<b", raw)[0],
}


class ReactionError(Exception):
    """Raised when a reaction packet cannot be decoded."""


@dataclass(frozen=True, slots=True)
class ReactionEvent:
    ticket: int
    code: int
    joint: str
    command: str
    values: tuple[int | float, ...]


def decode_reaction(data: bytes) -> ReactionEvent:
    """Decode a reaction payload (trailing frame padding is ignored)."""
    if len(data) < _HEADER.size:
        raise ReactionError(f"reaction too short for header: {len(data)} bytes")
    ticket, code, joint, command_len, values_len = _HEADER.unpack_from(data)
    command_end = _HEADER.size + command_len
    end = command_end + values_len * _VALUE_SIZE
    if len(data) < end:
        raise ReactionError(f"reaction too short: need {end} bytes, got {len(data)}")

    values = []
    for offset in range(command_end, end, _VALUE_SIZE):
        tag = data[offset]
        decoder = _DECODERS.get(tag)
        if decoder is None:
            raise ReactionError(f"unknown value type {chr(tag)!r}")
        values.append(decoder(data[offset + 1 : offset + _VALUE_SIZE]))

    return ReactionEvent(
        ticket=ticket,
        code=code,
        joint=_c_string(joint),
        command=data[_HEADER.size : command_end].decode(errors="replace"),
        values=tuple(values),
    )


@dataclass(frozen=True, slots=True)
class Reaction:
    """Calls ``callback`` for events on ``joint`` and ``command`` with one of ``codes``.

    An empty ``codes`` matches any code. Reactions are bound to tickets by
    ``ReactionRegistry``, so one ``Reaction`` can be reused across tickets.
    """

    joint: str
    command: str
    codes: Collection[int]
    callback: Callable[[ReactionEvent], object]

    def __post_init__(self) -> None:
        object.__setattr__(self, "command", str(self.command))
        object.__setattr__(self, "codes", frozenset(self.codes))

    def matches(self, event: ReactionEvent) -> bool:
        return (
            event.joint == self.joint
            and event.command == self.command
            and (not self.codes or event.code in self.codes)
        )


class ReactionRegistry:
    """Thread-safe mapping of tickets to their reactions."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._by_ticket: dict[int, list[Reaction]] = {}

    def add(self, ticket: int, reactions: Iterable[Reaction]) -> None:
        with self._lock:
            self._by_ticket.setdefault(ticket, []).extend(reactions)

    def remove_ticket(self, ticket: int) -> None:
        with self._lock:
            self._by_ticket.pop(ticket, None)

    def dispatch(self, event: ReactionEvent) -> int:
        """Run every matching callback for ``event``; return how many ran.

        Callbacks run outside the lock, so they may add or remove reactions.
        """
        with self._lock:
            matching = [
                r for r in self._by_ticket.get(event.ticket, ()) if r.matches(event)
            ]
        if not matching:
            logger.debug("No reaction for %s", event)
        for reaction in matching:
            try:
                reaction.callback(event)
            except Exception:
                logger.exception("Reaction callback failed for %s", event)
        return len(matching)


def _c_string(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode(errors="replace")
