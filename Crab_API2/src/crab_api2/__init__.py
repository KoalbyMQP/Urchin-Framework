"""Crab API: host-side controller for the Urchin firmware.

from crab_api2 import Crab, TicketType, herkulex
from crab_api2.herkulex import JogLedColor

with Crab("bridge.toml") as crab:
    crab.send(TicketType.ASYNCHRONOUS, [herkulex.move_one("Shoulder", 23.0, 40, JogLedColor.LED_RED)])
"""

from crab_api2 import herkulex
from crab_api2.bridge import BridgeError, load_bridge
from crab_api2.crab import (
    Crab,
    CrabError,
    NoFreeTicketError,
    ReplyTimeoutError,
    TicketType,
    UnknownJointError,
)
from crab_api2.link import LinkError
from crab_api2.reaction import Reaction, ReactionError, ReactionEvent

__all__ = [
    "BridgeError",
    "Crab",
    "CrabError",
    "LinkError",
    "NoFreeTicketError",
    "Reaction",
    "ReactionError",
    "ReactionEvent",
    "ReplyTimeoutError",
    "TicketType",
    "UnknownJointError",
    "herkulex",
    "load_bridge",
]
