"""Brand-neutral motor commands and their ``LoadTicket`` encoding.

A ``Command`` is addressed to a joint by name and carries a command name and up to
``MAX_VALUES`` values. ``Command.pack`` builds the ``LoadTicket`` packet parsed by
``Urchin_Firmware/src/Conversation/UnPacker.cpp::LoadTicket``::

    u32 ticket | char joint[50] | u8 command_len | u8 values_len | command | values

Each value is 5 bytes: a type tag (``I``/``F``/``B``) followed by a 4-byte union.
The firmware reads the union according to the command, not the tag.
"""

from __future__ import annotations

import struct
from collections.abc import Sequence
from dataclasses import dataclass, field
from typing import TypeAlias

from crab_api2.bridge import MAX_JOINT

MAX_VALUES = 20  # MAX_VALUES_LEN in UnPacker.h
MAX_NAME = 254  # LoadTicket copies the name into char command[255]

Value: TypeAlias = int | float | bool

_HEADER = struct.Struct(f"<I{MAX_JOINT}sBB")
_INT = struct.Struct("<ci")
_FLOAT = struct.Struct("<cf")
_BOOL = struct.Struct("<c?3x")
_INT32_MIN = -(2**31)
_INT32_MAX = 2**31 - 1
_UINT32_MAX = 2**32 - 1


def encode_value(value: Value) -> bytes:
    """Encode one value as a 5-byte tagged union."""
    # bool is a subclass of int, so it must be checked first
    if isinstance(value, bool):
        return _BOOL.pack(b"B", value)
    if isinstance(value, int):
        if not _INT32_MIN <= value <= _INT32_MAX:
            raise ValueError(f"int value must fit in a signed 32-bit int, got {value}")
        return _INT.pack(b"I", value)
    if isinstance(value, float):
        try:
            return _FLOAT.pack(b"F", value)
        except OverflowError:
            raise ValueError(
                f"float value must fit in a 32-bit float, got {value}"
            ) from None
    raise TypeError(f"values must be int, float, or bool, got {type(value).__name__}")


@dataclass(frozen=True, slots=True)
class Command:
    """One command for one joint, to be loaded onto a ticket."""

    joint: str
    name: str
    values: Sequence[Value] = ()
    _encoded_values: bytes = field(init=False, repr=False, compare=False)

    def __post_init__(self) -> None:
        if not self.joint:
            raise ValueError("joint must not be empty")
        _check_length("joint", self.joint, MAX_JOINT - 1)
        if not self.name:
            raise ValueError("name must not be empty")
        _check_length("name", self.name, MAX_NAME)
        values = tuple(self.values)
        if len(values) > MAX_VALUES:
            raise ValueError(
                f"a command takes at most {MAX_VALUES} values, got {len(values)}"
            )
        # Encode now so bad values fail at construction rather than at send time
        encoded = b"".join(encode_value(v) for v in values)
        object.__setattr__(self, "values", values)
        object.__setattr__(self, "_encoded_values", encoded)

    def pack(self, ticket: int) -> bytes:
        """Encode as a ``LoadTicket`` packet for ``ticket``."""
        if not 0 <= ticket <= _UINT32_MAX:
            raise ValueError(
                f"ticket must be between 0 and {_UINT32_MAX}, got {ticket}"
            )
        name = self.name.encode()
        header = _HEADER.pack(ticket, self.joint.encode(), len(name), len(self.values))
        return b"LoadTicket" + header + name + self._encoded_values


def _check_length(label: str, value: str, limit: int) -> None:
    length = len(value.encode())
    if length > limit:
        raise ValueError(f"{label} must be at most {limit} bytes (UTF-8), got {length}")
