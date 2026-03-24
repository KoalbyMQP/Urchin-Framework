import pytest
from src.Bridge import Bridge, TomlValueError


def test_Bridge_Bad_Version_Missing():
#"MotorNum","MotorBrand","MotorModel","JointName","Bounds",
    MonoPairs = ["Bounds", "AlignmentAngle"]

    for Solo in MonoPairs:

        with pytest.raises(TomlValueError) as exc_info:
            Bridge("test/FakeFiles/Names/"+str(Solo)+".toml")
        assert str(exc_info.value.args[0]) == f"\"Version\" is missing from Motor"