import pytest
import marshaltype_py
import inspect

class TestItem:

    def test_setup(self):
        item = marshaltype_py.Item()
        methods = [attr for attr in dir(item) if callable(getattr(item, attr))]
        print(methods)

    def test_eq(self):
        item1 = marshaltype_py.Item(
            "joint_1",
            "move",
            [1, 2.5, True]
        )

        item2 = marshaltype_py.Item(
            "joint_1",
            "move",
            [1, 2.5, True]
        )

        assert item1 == item2

    def test_marshal_demarshal(self):
        item1 = marshaltype_py.Item(
            "joint_1",
            "move",
            [1, 2.5, True]
        )

        err, data = item1.Marshal()
        assert err == 0

        item2 = marshaltype_py.Item()
        err = item2.DeMarshal(data)
        assert err == 0

        assert item1 == item2

    def test_values_assignment(self):
        item = marshaltype_py.Item()
        item.values = [True, False, 1, 2, 3.5]

        assert item.values == [True, False, 1, 2, 3.5]
    def test_plain_python_values_match_cpp_bytes(self):
        # Same command built from plain Python numbers must produce exactly the
        # bytes the C++ side makes for Item("left_knee", "Move", 30.0f, 1000):
        # 50-byte joint, cmd len, value count, "Move", 'F' 30.0, 'I' 1000.
        item = marshaltype_py.Item("left_knee", "Move", [30.0, 1000])
        err, data = item.Marshal()
        assert err == 0
        expected = (b"left_knee".ljust(50, b"\x00") + bytes([4, 2]) + b"Move"
                    + bytes.fromhex("460000F041") + bytes.fromhex("49E8030000"))
        assert bytes(data) == expected

    def test_repr(self):
        item = marshaltype_py.Item("left_knee", "Move", [30.0, 1000])
        text = repr(item)
        assert "left_knee" in text and "Move" in text

    def test_rejects_unsupported_value(self):
        with pytest.raises(TypeError):
            marshaltype_py.Item("left_knee", "Move", ["thirty"])
