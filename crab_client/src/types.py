from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, IntEnum
from typing import Iterable, Sequence


class TicketType(str, Enum):
    SEQUENTIAL = "S"
    RESOLVING = "R"
    ASYNCHRONOUS = "A"
    INTERRUPT = "I"


class HealthMetric(str, Enum):
    TOTAL_RAM = "TotalRam"
    FREE_RAM = "FreeRam"
    CPU = "CPU"
    ACK_CHECK = "AckCheck"


class ValueKind(IntEnum):
    INT = 1
    FLOAT = 2
    BOOL = 3


@dataclass(frozen=True)
class Value:
    kind: ValueKind
    int_value: int = 0
    float_value: float = 0.0
    bool_value: bool = False

    @classmethod
    def from_python(cls, value: object) -> "Value":
        if isinstance(value, bool):
            return cls(kind=ValueKind.BOOL, bool_value=value)
        if isinstance(value, int):
            return cls(kind=ValueKind.INT, int_value=value)
        if isinstance(value, float):
            return cls(kind=ValueKind.FLOAT, float_value=value)
        raise TypeError(f"Unsupported command value type: {type(value).__name__}")


@dataclass(frozen=True)
class CommandItem:
    joint: str
    command: str
    values: Sequence[Value]

    @classmethod
    def from_primitives(cls, joint: str, command: str, values: Iterable[object]) -> "CommandItem":
        return cls(joint=joint, command=command, values=tuple(Value.from_python(value) for value in values))
