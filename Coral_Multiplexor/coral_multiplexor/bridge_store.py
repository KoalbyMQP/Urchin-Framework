from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import struct
import tomllib


class BridgeValidationError(ValueError):
    pass


@dataclass(frozen=True)
class MotorDefinition:
    number: int
    brand: str
    model: str
    joint: str
    bounds: tuple[int, int]
    alignment_angle: int


class BridgeStore:
    VERSION = "0.1"
    _DATA_STRUCT = struct.Struct("<B30s10s50siii?")

    def __init__(self, source_path: str, motors: list[MotorDefinition], version: str) -> None:
        self.source_path = source_path
        self.motors = motors
        self.version = version

    @classmethod
    def load(cls, source_path: str) -> "BridgeStore":
        path = Path(source_path)
        with path.open("rb") as handle:
            document = tomllib.load(handle)

        meta = document.get("meta", {})
        version = meta.get("Version")
        if version != cls.VERSION:
            raise BridgeValidationError(f"File version mismatch: expected {cls.VERSION}, found {version}")

        motor_sections = document.get("Motor")
        if not isinstance(motor_sections, list) or not motor_sections:
            raise BridgeValidationError('"Motor" entries are missing in .toml')

        motors = [cls._parse_motor(index, motor) for index, motor in enumerate(motor_sections, start=1)]
        return cls(str(path.resolve()), motors, version)

    @classmethod
    def _parse_motor(cls, index: int, motor: dict[str, object]) -> MotorDefinition:
        required = ["MotorNum", "MotorBrand", "MotorModel", "JointName", "Bounds", "AlignmentAngle"]
        missing = [field for field in required if field not in motor]
        if missing:
            raise BridgeValidationError(f"Motor {index} is missing fields: {', '.join(missing)}")

        bounds = motor["Bounds"]
        if not isinstance(bounds, list) or len(bounds) != 2 or not all(isinstance(value, int) for value in bounds):
            raise BridgeValidationError(f'Motor {index} has invalid "Bounds"; expected [int, int]')

        try:
            return MotorDefinition(
                number=int(motor["MotorNum"]),
                brand=str(motor["MotorBrand"]),
                model=str(motor["MotorModel"]),
                joint=str(motor["JointName"]),
                bounds=(int(bounds[0]), int(bounds[1])),
                alignment_angle=int(motor["AlignmentAngle"]),
            )
        except (TypeError, ValueError) as exc:
            raise BridgeValidationError(f"Motor {index} contains invalid field types") from exc

    def encode_bridge_packets(self) -> list[bytes]:
        packets: list[bytes] = []
        for index, motor in enumerate(self.motors, start=1):
            payload = self._DATA_STRUCT.pack(
                motor.number,
                motor.brand.encode("utf-8")[:30].ljust(30, b"\x00"),
                motor.model.encode("utf-8")[:10].ljust(10, b"\x00"),
                motor.joint.encode("utf-8")[:50].ljust(50, b"\x00"),
                motor.bounds[0],
                motor.bounds[1],
                motor.alignment_angle,
                index == len(self.motors),
            )
            packets.append(b"BridgeAdd" + payload)
        return packets

    def get_motor(self, joint: str) -> MotorDefinition:
        for motor in self.motors:
            if motor.joint == joint:
                return motor
        raise KeyError(f"Unknown joint {joint}")
