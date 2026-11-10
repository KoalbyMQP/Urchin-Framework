from pathlib import Path

import pytest

from crab_api2.bridge import BridgeError, Motor, load_bridge, parse_bridge

FIXTURES = Path(__file__).parent / "fixtures"

HEADER = '[meta]\nVersion = "0.1"\n'

SHOULDER = """
[[Motor]]
MotorNum = 7
MotorBrand = "HerkuleX"
MotorModel = "0601"
JointName = "Shoulder"
Bounds = [-45, 45]
AlignmentAngle = 0
"""


def motor_toml(**overrides: str | None) -> str:
    """Return a [[Motor]] table; each override replaces a field's TOML value, None removes it."""
    fields = {
        "MotorNum": "7",
        "MotorBrand": '"HerkuleX"',
        "MotorModel": '"0601"',
        "JointName": '"Shoulder"',
        "Bounds": "[-45, 45]",
        "AlignmentAngle": "0",
    }
    fields.update(overrides)
    body = "\n".join(
        f"{key} = {value}" for key, value in fields.items() if value is not None
    )
    return f"\n[[Motor]]\n{body}\n"


# --- valid files ---


def test_load_sample_file():
    motors = load_bridge(FIXTURES / "bridge.toml")
    assert motors == [
        Motor(7, "HerkuleX", "0601", "Shoulder", (-45, 45), 0),
        Motor(8, "HerkuleX", "0602", "Hip", (-50, 50), 0),
    ]


def test_load_accepts_str_path():
    assert len(load_bridge(str(FIXTURES / "bridge.toml"))) == 2


def test_parse_single_motor():
    assert parse_bridge(HEADER + SHOULDER) == [
        Motor(7, "HerkuleX", "0601", "Shoulder", (-45, 45), 0)
    ]


def test_limits_are_inclusive():
    toml = HEADER + motor_toml(
        MotorNum="253",
        MotorBrand=f'"{"b" * 29}"',
        MotorModel=f'"{"m" * 9}"',
        JointName=f'"{"j" * 49}"',
        Bounds="[10, 10]",
    )
    assert parse_bridge(toml)[0].number == 253


# --- file-level errors ---


def test_missing_file():
    with pytest.raises(FileNotFoundError):
        load_bridge(FIXTURES / "does_not_exist.toml")


def test_invalid_toml_syntax():
    with pytest.raises(BridgeError, match="Invalid TOML"):
        parse_bridge("[meta\nVersion = ")


def test_load_error_carries_path(tmp_path):
    bad = tmp_path / "bad.toml"
    bad.write_text("[meta]\n")
    with pytest.raises(BridgeError) as exc_info:
        load_bridge(bad)
    assert exc_info.value.path == bad
    assert str(bad) in str(exc_info.value)


def test_version_missing():
    with pytest.raises(BridgeError, match=r"\[meta\] Version is missing"):
        parse_bridge("[meta]\n" + SHOULDER)


def test_meta_missing():
    with pytest.raises(BridgeError, match=r"\[meta\] Version is missing"):
        parse_bridge(SHOULDER)


def test_version_wrong_type():
    with pytest.raises(BridgeError, match="Version must be a string, got float"):
        parse_bridge("[meta]\nVersion = 0.1\n" + SHOULDER)


def test_version_mismatch():
    with pytest.raises(
        BridgeError, match=r"Unsupported bridge version '0\.0' \(expected '0\.1'\)"
    ):
        parse_bridge('[meta]\nVersion = "0.0"\n' + SHOULDER)


def test_no_motors():
    with pytest.raises(BridgeError, match=r"No \[\[Motor\]\] tables found"):
        parse_bridge(HEADER)


def test_misspelled_motor_table():
    with pytest.raises(BridgeError, match=r"No \[\[Motor\]\] tables found"):
        parse_bridge(HEADER + SHOULDER.replace("[[Motor]]", "[[Moter]]"))


def test_motor_not_array_of_tables():
    with pytest.raises(BridgeError, match=r"Motor must be an array of tables"):
        parse_bridge("Motor = 5\n" + HEADER)


# --- per-motor field errors ---


@pytest.mark.parametrize(
    "field",
    ["MotorNum", "MotorBrand", "MotorModel", "JointName", "Bounds", "AlignmentAngle"],
)
def test_missing_field(field):
    with pytest.raises(BridgeError, match=f"Motor #1.*missing field '{field}'"):
        parse_bridge(HEADER + motor_toml(**{field: None}))


def test_unknown_field():
    toml = HEADER + motor_toml(Bounds=None) + "Bound = [-45, 45]\n"
    with pytest.raises(
        BridgeError, match=r"Motor #1 \(Shoulder\): unknown field 'Bound'"
    ):
        parse_bridge(toml)


def test_error_names_correct_motor():
    toml = (
        HEADER + SHOULDER + motor_toml(JointName='"Hip"', MotorNum="8", Bounds='"wide"')
    )
    with pytest.raises(BridgeError, match=r"Motor #2 \(Hip\): Bounds"):
        parse_bridge(toml)


@pytest.mark.parametrize(
    ("field", "value", "type_name"),
    [
        ("MotorNum", '"7"', "str"),
        ("MotorNum", "7.0", "float"),
        ("MotorNum", "true", "bool"),
        ("MotorBrand", "5", "int"),
        ("MotorModel", "601", "int"),
        ("JointName", "false", "bool"),
        ("AlignmentAngle", "0.5", "float"),
    ],
)
def test_wrong_field_type(field, value, type_name):
    with pytest.raises(
        BridgeError, match=f"{field} must be an? (int|str), got {type_name}"
    ):
        parse_bridge(HEADER + motor_toml(**{field: value}))


@pytest.mark.parametrize(
    "bounds", ['"wide"', "[-45]", "[-45, 0, 45]", "[-45.0, 45.0]", "[true, false]"]
)
def test_bad_bounds_shape(bounds):
    with pytest.raises(BridgeError, match=r"Bounds must be a list of 2 ints"):
        parse_bridge(HEADER + motor_toml(Bounds=bounds))


def test_bounds_order():
    with pytest.raises(BridgeError, match=r"Bounds min \(45\) must be <= max \(-45\)"):
        parse_bridge(HEADER + motor_toml(Bounds="[45, -45]"))


# --- value limits ---


@pytest.mark.parametrize("number", ["-1", "254", "300"])
def test_motor_num_out_of_range(number):
    with pytest.raises(
        BridgeError, match=f"MotorNum must be between 0 and 253, got {number}"
    ):
        parse_bridge(HEADER + motor_toml(MotorNum=number))


@pytest.mark.parametrize(
    ("field", "limit"), [("MotorBrand", 29), ("MotorModel", 9), ("JointName", 49)]
)
def test_string_too_long(field, limit):
    toml = HEADER + motor_toml(**{field: f'"{"x" * (limit + 1)}"'})
    with pytest.raises(BridgeError, match=f"{field} must be at most {limit} bytes"):
        parse_bridge(toml)


def test_string_length_counts_utf8_bytes():
    # 25 two-byte characters = 50 bytes, over the 49-byte JointName limit
    with pytest.raises(BridgeError, match="JointName must be at most 49 bytes"):
        parse_bridge(HEADER + motor_toml(JointName=f'"{"é" * 25}"'))


@pytest.mark.parametrize(
    ("field", "value"),
    [
        ("Bounds", "[-2147483649, 0]"),
        ("Bounds", "[0, 2147483648]"),
        ("AlignmentAngle", "2147483648"),
    ],
)
def test_int32_overflow(field, value):
    with pytest.raises(BridgeError, match="32-bit"):
        parse_bridge(HEADER + motor_toml(**{field: value}))


# --- duplicates ---


def test_duplicate_joint_name():
    toml = HEADER + SHOULDER + motor_toml(MotorNum="8")
    with pytest.raises(
        BridgeError,
        match=r"Motor #2 \(Shoulder\): duplicate JointName 'Shoulder' \(also Motor #1\)",
    ):
        parse_bridge(toml)


def test_duplicate_motor_num():
    toml = HEADER + SHOULDER + motor_toml(JointName='"Hip"')
    with pytest.raises(
        BridgeError, match=r"Motor #2 \(Hip\): duplicate MotorNum 7 \(also Motor #1\)"
    ):
        parse_bridge(toml)


# --- Motor constructed directly ---


def test_motor_validates_on_construction():
    with pytest.raises(ValueError, match="MotorNum must be between 0 and 253"):
        Motor(254, "HerkuleX", "0601", "Shoulder", (-45, 45), 0)


def test_motor_is_immutable():
    motor = Motor(7, "HerkuleX", "0601", "Shoulder", (-45, 45), 0)
    with pytest.raises(AttributeError):
        motor.number = 8  # type: ignore[misc]


def test_empty_joint_name():
    with pytest.raises(BridgeError, match="Motor #1: JointName must not be empty"):
        parse_bridge(HEADER + motor_toml(JointName='""'))
