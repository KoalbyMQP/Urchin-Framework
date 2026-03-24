import pytest
from src.Bridge import Bridge, TomlValueError


def test_Bridge_Bad_Motor_Missing():
    with pytest.raises(TomlValueError) as exc_info:
        Bridge("test/FakeFiles/motor/MotorMissing.toml")
    assert str(exc_info.value.args[0]) == f"\"Motor\"s are missing in .toml"