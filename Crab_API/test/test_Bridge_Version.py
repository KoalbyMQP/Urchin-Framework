import pytest
from src.Bridge import Bridge, TomlValueError


def test_Bridge_Bad_Version_Missing():
    with pytest.raises(TomlValueError) as exc_info:
        Bridge("test/FakeFiles/Version/missing.toml")
    assert str(exc_info.value.args[0]) == f"\"Version\" is missing in .toml"

def test_Bridge_Bad_Version_Type():
    with pytest.raises(TomlValueError) as exc_info:
        Bridge("test/FakeFiles/Version/type.toml")
    assert str(exc_info.value.args[0]) == f"Version in .toml must be a string, got float"
    assert exc_info.value.line_number == 2

def test_Bridge_Bad_Version_Number():
    with pytest.raises(TomlValueError) as exc_info:
        Bridge("test/FakeFiles/Version/number.toml")
    assert str(exc_info.value.args[0]) == f"File version mismatch: expected 0.1, found 0.0"
    assert exc_info.value.line_number == 2