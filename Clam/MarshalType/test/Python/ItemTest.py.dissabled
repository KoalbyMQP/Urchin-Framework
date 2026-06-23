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