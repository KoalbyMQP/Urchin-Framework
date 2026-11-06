"""Bridge file parsing and encoding.

A bridge file maps physical motors to URDF joint names:

    [meta]
    Version = "0.1"

    [[Motor]]
    MotorNum = 7              # number printed on the motor
    MotorBrand = "HerkuleX"
    MotorModel = "0601"
    JointName = "Shoulder"    # joint name from the URDF, exactly
    Bounds = [-45, 45]        # joint limits from CAD
    AlignmentAngle = 0        # angle the motor should be at for assembly

Each motor is sent to the firmware as a ``BridgeAdd`` packet whose payload is the
packed ``BridgeMotor`` struct from ``Urchin_Firmware/src/Global/Bridge.h``.
"""

from __future__ import annotations

import logging
import struct
import tomllib
from dataclasses import dataclass
from os import PathLike
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)

BRIDGE_VERSION = "0.1"

# Buffer sizes from Bridge.h. The firmware overwrites the last byte with '\0',
# so the usable length of each string is one less.
MAX_BRAND = 30
MAX_MODEL = 10
MAX_JOINT = 50
MAX_MOTOR_NUM = 253

_INT32_MIN = -(2**31)
_INT32_MAX = 2**31 - 1

# Packed BridgeMotor: Num, Brand, Model, Joint, BoundsMin, BoundsMax, AlignmentAngle, NextNode
_PAYLOAD = struct.Struct(f"<B{MAX_BRAND}s{MAX_MODEL}s{MAX_JOINT}siii?")
_PACKET_HEADER = b"BridgeAdd" + struct.pack("<I", _PAYLOAD.size)

# TOML key -> expected type, in the order fields are reported
_MOTOR_FIELDS: dict[str, type] = {
    "MotorNum": int,
    "MotorBrand": str,
    "MotorModel": str,
    "JointName": str,
    "Bounds": list,
    "AlignmentAngle": int,
}


class BridgeError(Exception):
    """Raised when a bridge file is malformed or contains invalid values."""

    def __init__(self, message: str, path: Path | None = None) -> None:
        super().__init__(message)
        self.message = message
        self.path = path

    def __str__(self) -> str:
        return f"{self.path}: {self.message}" if self.path else self.message


@dataclass(frozen=True, slots=True)
class Motor:
    """One motor-to-joint mapping from a bridge file."""

    number: int
    brand: str
    model: str
    joint: str
    bounds: tuple[int, int]
    alignment_angle: int

    def __post_init__(self) -> None:
        if not 0 <= self.number <= MAX_MOTOR_NUM:
            raise ValueError(
                f"MotorNum must be between 0 and {MAX_MOTOR_NUM}, got {self.number}"
            )
        _check_length("MotorBrand", self.brand, MAX_BRAND)
        _check_length("MotorModel", self.model, MAX_MODEL)
        _check_length("JointName", self.joint, MAX_JOINT)
        # The firmware's GetBridge() rejects empty names, so the motor could never be addressed
        if not self.joint:
            raise ValueError("JointName must not be empty")
        low, high = self.bounds
        _check_int32("Bounds", low)
        _check_int32("Bounds", high)
        if low > high:
            raise ValueError(f"Bounds min ({low}) must be <= max ({high})")
        _check_int32("AlignmentAngle", self.alignment_angle)

    def pack(self, last: bool) -> bytes:
        """Encode as a ``BridgeAdd`` packet; ``last`` marks the final motor in the bridge."""
        return _PACKET_HEADER + _PAYLOAD.pack(
            self.number,
            self.brand.encode(),
            self.model.encode(),
            self.joint.encode(),
            self.bounds[0],
            self.bounds[1],
            self.alignment_angle,
            last,
        )


def load_bridge(path: str | PathLike[str]) -> list[Motor]:
    """Read and validate a bridge file."""
    path = Path(path)
    try:
        motors = parse_bridge(path.read_text(encoding="utf-8"))
    except BridgeError as e:
        raise BridgeError(e.message, path) from None
    for motor in motors:
        logger.debug("Loaded %s", motor)
    return motors


def parse_bridge(text: str) -> list[Motor]:
    """Parse and validate the contents of a bridge file."""
    try:
        data = tomllib.loads(text)
    except tomllib.TOMLDecodeError as e:
        raise BridgeError(f"Invalid TOML: {e}") from None

    _check_version(data)

    tables = data.get("Motor")
    if tables is None or tables == []:
        raise BridgeError("No [[Motor]] tables found")
    if not isinstance(tables, list) or not all(isinstance(t, dict) for t in tables):
        raise BridgeError("Motor must be an array of tables ([[Motor]])")

    motors: list[Motor] = []
    joints: dict[str, int] = {}
    numbers: dict[int, int] = {}
    for index, table in enumerate(tables, start=1):
        motor = _parse_motor(table, index)
        prefix = _motor_prefix(table, index)
        if motor.joint in joints:
            raise BridgeError(
                f"{prefix}duplicate JointName '{motor.joint}' (also Motor #{joints[motor.joint]})"
            )
        if motor.number in numbers:
            raise BridgeError(
                f"{prefix}duplicate MotorNum {motor.number} (also Motor #{numbers[motor.number]})"
            )
        joints[motor.joint] = index
        numbers[motor.number] = index
        motors.append(motor)
    return motors


def _check_version(data: dict[str, Any]) -> None:
    meta = data.get("meta")
    if not isinstance(meta, dict) or "Version" not in meta:
        raise BridgeError("[meta] Version is missing")
    version = meta["Version"]
    if not isinstance(version, str):
        raise BridgeError(
            f"[meta] Version must be a string, got {type(version).__name__}"
        )
    if version != BRIDGE_VERSION:
        raise BridgeError(
            f"Unsupported bridge version {version!r} (expected {BRIDGE_VERSION!r})"
        )


def _parse_motor(table: dict[str, Any], index: int) -> Motor:
    prefix = _motor_prefix(table, index)

    for key in table:
        if key not in _MOTOR_FIELDS:
            raise BridgeError(f"{prefix}unknown field '{key}'")
    for key, expected in _MOTOR_FIELDS.items():
        if key not in table:
            raise BridgeError(f"{prefix}missing field '{key}'")
        value = table[key]
        # bool is a subclass of int, so compare exact types
        if type(value) is not expected:
            if expected is list:
                raise BridgeError(f"{prefix}Bounds must be a list of 2 ints")
            article = "an" if expected is int else "a"
            raise BridgeError(
                f"{prefix}{key} must be {article} {expected.__name__}, got {type(value).__name__}"
            )

    bounds = table["Bounds"]
    if len(bounds) != 2 or any(type(b) is not int for b in bounds):
        raise BridgeError(f"{prefix}Bounds must be a list of 2 ints")

    try:
        return Motor(
            number=table["MotorNum"],
            brand=table["MotorBrand"],
            model=table["MotorModel"],
            joint=table["JointName"],
            bounds=(bounds[0], bounds[1]),
            alignment_angle=table["AlignmentAngle"],
        )
    except ValueError as e:
        raise BridgeError(f"{prefix}{e}") from None


def _motor_prefix(table: dict[str, Any], index: int) -> str:
    joint = table.get("JointName")
    return (
        f"Motor #{index} ({joint}): "
        if isinstance(joint, str) and joint
        else f"Motor #{index}: "
    )


def _check_length(name: str, value: str, size: int) -> None:
    length = len(value.encode())
    if length > size - 1:
        raise ValueError(
            f"{name} must be at most {size - 1} bytes (UTF-8), got {length}"
        )


def _check_int32(name: str, value: int) -> None:
    if not _INT32_MIN <= value <= _INT32_MAX:
        raise ValueError(f"{name} must fit in a signed 32-bit int, got {value}")
