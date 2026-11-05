"""The Crab client: ticket protocol, health checks, and reaction dispatch.

A ticket is sent in three steps (``Urchin_Firmware/src/Conversation/UnPacker.cpp``)::

    FormatTicket<type>    -> E: i32 error [, u32 ticket]
    LoadTicket ...        -> one packet per command (see ``Command.pack``)
    PunchTicket<u32>      -> run the ticket

Two background threads run while a ``Crab`` is open: a reader that routes incoming
frames by stream, and a dispatcher that runs reaction callbacks. Callbacks run on the
dispatcher, so they may call ``send`` without blocking the reader.
"""

from __future__ import annotations

import logging
import queue
import struct
import threading
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from enum import StrEnum
from os import PathLike
from typing import Literal, Self, overload

from crab_api2.bridge import load_bridge
from crab_api2.command import Command
from crab_api2.link import Link
from crab_api2.reaction import (
    Reaction,
    ReactionError,
    ReactionEvent,
    ReactionRegistry,
    decode_reaction,
)

logger = logging.getLogger(__name__)
firmware_logger = logging.getLogger("crab_api2.firmware")

VPID = 0
URCHIN_OK = 0
URCHIN_ERROR_NO_FREE_TICKET = 1  # Global/Errors.h

type HealthCheck = Literal["FreeRam", "TotalRam", "AckCheck"]
HEALTH_CHECKS: tuple[HealthCheck, ...] = ("FreeRam", "TotalRam", "AckCheck")

_ACK_CHECKING = b"GetHealth:AckCheck:Checking "
_ACK_DONE = b"AckCheck:done"


class TicketType(StrEnum):
    SEQUENTIAL = "S"
    RESOLVING = "R"
    ASYNCHRONOUS = "A"
    INTERRUPT = "I"


class CrabError(Exception):
    """Base class for Crab client errors."""


class NoFreeTicketError(CrabError):
    """The firmware has no free ticket slots."""


class ReplyTimeoutError(CrabError):
    """The firmware did not reply in time."""


class UnknownJointError(CrabError):
    """A command names a joint that is not in the bridge."""


@dataclass(frozen=True, slots=True)
class MotorStatus:
    """One motor's result from the ``AckCheck`` health check."""

    joint: str
    number: int
    responding: bool
    status_detail: int
    status_error: int


class Crab:
    """A connection to an Urchin board.

    ``url`` is passed to ``Link.open``: an explicit device or pyserial URL, else
    ``$URCHIN_PORT``, else the first board found by discovery.
    """

    def __init__(
        self,
        bridge_path: str | PathLike[str],
        url: str | None = None,
        *,
        reply_timeout: float = 1.0,
    ) -> None:
        self.motors = load_bridge(bridge_path)
        self.reply_timeout = reply_timeout
        self._joints = {motor.joint: motor for motor in self.motors}
        self._numbers = {motor.number: motor for motor in self.motors}

        self._reactions = ReactionRegistry()
        self._exchange: queue.Queue[bytes] = queue.Queue()
        self._events: queue.Queue[ReactionEvent | None] = queue.Queue()
        self._debug_listeners: list[queue.Queue[bytes]] = []
        self._listeners_lock = threading.Lock()
        # One request/reply exchange at a time, so replies on E can't interleave
        self._request_lock = threading.Lock()
        self._stop = threading.Event()
        self._closed = False

        self._link = Link.open(url)
        self._reader = threading.Thread(
            target=self._read_loop, name="crab-reader", daemon=True
        )
        self._dispatcher = threading.Thread(
            target=self._dispatch_loop, name="crab-dispatcher", daemon=True
        )
        self._reader.start()
        self._dispatcher.start()
        try:
            self._upload_bridge()
        except BaseException:
            self.close()
            raise

    # --- public API ---

    def send(
        self,
        ticket_type: TicketType,
        commands: Sequence[Command],
        reactions: Iterable[Reaction] = (),
    ) -> int:
        """Send ``commands`` as one ticket and return its ticket number.

        ``reactions`` stay registered for the ticket until ``close_ticket``.
        """
        for command in commands:
            if command.joint not in self._joints:
                known = ", ".join(self._joints)
                raise UnknownJointError(
                    f"joint {command.joint!r} is not in the bridge (known: {known})"
                )
        ticket_type = TicketType(ticket_type)
        reactions = list(reactions)

        with self._request_lock:
            self._drain_exchange()
            self._link.send(VPID, b"FormatTicket" + ticket_type.encode())
            error = struct.unpack_from("<i", self._reply("FormatTicket"))[0]
            if error == URCHIN_ERROR_NO_FREE_TICKET:
                raise NoFreeTicketError("the firmware has no free tickets")
            if error != URCHIN_OK:
                raise CrabError(f"FormatTicket failed with error code {error}")
            ticket = struct.unpack_from("<I", self._reply("FormatTicket"))[0]
            logger.debug("Formatted ticket %d (%s)", ticket, ticket_type.name)

            # Register before loading: a command can trigger a reaction while it loads
            if reactions:
                self._reactions.add(ticket, reactions)
            for command in commands:
                self._link.send(VPID, command.pack(ticket))
            self._link.send(VPID, b"PunchTicket" + struct.pack("<I", ticket))
        logger.debug("Punched ticket %d with %d command(s)", ticket, len(commands))
        return ticket

    def close_ticket(self, ticket: int) -> None:
        """Stop dispatching reactions for ``ticket``."""
        self._reactions.remove_ticket(ticket)
        # TODO: send "CloseTicket" to the firmware once it implements it
        # (currently a stub in UnPacker.cpp)

    def ticket_info(self, ticket: int) -> None:
        # TODO: request the ticket's status once the firmware implements "TicketInfo"
        # (currently a stub in UnPacker.cpp)
        raise NotImplementedError("TicketInfo is not implemented by the firmware yet")

    @overload
    def get_health(self, check: Literal["FreeRam", "TotalRam"]) -> int: ...
    @overload
    def get_health(self, check: Literal["AckCheck"]) -> dict[str, MotorStatus]: ...

    def get_health(self, check: HealthCheck) -> int | dict[str, MotorStatus]:
        """Run a firmware health check.

        ``FreeRam``: free heap bytes. ``TotalRam``: the firmware reports the minimum
        free heap ever seen. ``AckCheck``: status of each bridge motor, by joint.
        """
        if check not in HEALTH_CHECKS:
            raise ValueError(
                f"check must be one of {', '.join(HEALTH_CHECKS)}; got {check!r}"
            )
        if check == "AckCheck":
            return self._ack_check()
        with self._request_lock:
            self._drain_exchange()
            self._link.send(VPID, b"GetHealth" + check.encode())
            text = _c_string(self._reply(check))
        try:
            return int(text)
        except ValueError:
            raise CrabError(f"unexpected {check} reply: {text!r}") from None

    def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        self._stop.set()
        self._events.put(None)
        for thread in (self._reader, self._dispatcher):
            # close() may be called from a reaction callback, on the dispatcher itself
            if thread is not threading.current_thread():
                thread.join()
        self._link.close()

    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc_info: object) -> None:
        self.close()

    # --- internals ---

    def _upload_bridge(self) -> None:
        for index, motor in enumerate(self.motors):
            last = index == len(self.motors) - 1
            self._link.send(VPID, motor.pack(last))
            logger.debug("Sent bridge motor %d (%s)", motor.number, motor.joint)

    def _reply(self, request: str) -> bytes:
        try:
            return self._exchange.get(timeout=self.reply_timeout)
        except queue.Empty:
            raise ReplyTimeoutError(
                f"no reply to {request} within {self.reply_timeout} s"
            ) from None

    def _drain_exchange(self) -> None:
        # Drop late replies to earlier requests that timed out
        while True:
            try:
                stale = self._exchange.get_nowait()
            except queue.Empty:
                return
            logger.warning("Discarding stale reply: %r", stale.rstrip(b"\0")[:32])

    def _ack_check(self) -> dict[str, MotorStatus]:
        lines: queue.Queue[bytes] = queue.Queue()
        with self._listeners_lock:
            self._debug_listeners.append(lines)
        try:
            with self._request_lock:
                self._link.send(VPID, b"GetHealthAckCheck")
                return self._collect_ack_check(lines)
        finally:
            with self._listeners_lock:
                self._debug_listeners.remove(lines)

    def _collect_ack_check(self, lines: queue.Queue[bytes]) -> dict[str, MotorStatus]:
        statuses: dict[str, MotorStatus] = {}
        current: int | None = None
        while True:
            try:
                line = lines.get(timeout=self.reply_timeout)
            except queue.Empty:
                raise ReplyTimeoutError(
                    f"AckCheck did not finish within {self.reply_timeout} s of the last reply"
                ) from None
            if line == _ACK_DONE:
                return statuses
            if line.startswith(_ACK_CHECKING):
                number = line[len(_ACK_CHECKING) :]
                current = int(number) if number.isdigit() else None
                continue
            if current is None:
                continue
            # "HM<num>00" = missing; "HF<num><detail><error>", where detail and error are
            # raw bytes from "%c" and a zero byte ends the string early
            missing, found = b"HM%d" % current, b"HF%d" % current
            if line.startswith(missing):
                status = (False, 0, 0)
            elif line.startswith(found):
                rest = line[len(found) :]
                status = (True, rest[0] if rest else 0, rest[1] if len(rest) > 1 else 0)
            else:
                continue
            motor = self._numbers.get(current)
            if motor is None:
                logger.warning(
                    "AckCheck reported motor %d, which is not in the bridge", current
                )
            else:
                statuses[motor.joint] = MotorStatus(motor.joint, current, *status)
            current = None

    def _read_loop(self) -> None:
        while not self._stop.is_set():
            try:
                frame = self._link.read()
            except Exception:
                if not self._stop.is_set():
                    logger.exception("Serial read failed; reader stopping")
                return
            if frame is None:
                continue
            match frame.stream:
                case "E":
                    self._exchange.put(frame.data)
                case "R":
                    try:
                        self._events.put(decode_reaction(frame.data))
                    except ReactionError as e:
                        logger.warning("Dropping malformed reaction: %s", e)
                case "D":
                    line = frame.data.split(b"\0", 1)[0]
                    firmware_logger.debug("%s", line.decode(errors="replace"))
                    with self._listeners_lock:
                        for listener in self._debug_listeners:
                            listener.put(line)
                case stream:
                    logger.debug("Ignoring frame on stream %r", stream)

    def _dispatch_loop(self) -> None:
        while (event := self._events.get()) is not None:
            self._reactions.dispatch(event)


def _c_string(data: bytes) -> str:
    return data.split(b"\0", 1)[0].decode(errors="replace")
