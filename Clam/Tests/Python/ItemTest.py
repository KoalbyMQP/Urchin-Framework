import Clam
import unittest






class TestCalculator(unittest.TestCase):
    item = Clam.Item(
        "joint_1",
        "move",
        [1, 2.5, True]
    )

    print(item)
    def test_eq(self):
        item = Clam.Item(
            "joint_1",
            "move",
            [1, 2.5, True]
        )
    #self.assertTrue(item == item)


if __name__ == "__main__":
    item = Clam.Item(
        "joint_1",
        "move",
        [1, 2.5, True]
    )


    methods = [attr for attr in dir(item) if callable(getattr(item, attr))]
    print(methods)


    item.Marshal()
    err, data = item.Marshal()

    if err == 0:
        print(data)


    item2 = Clam.Item()
    item2.DeMarshal(data)
    print(item2)





    i = Clam.Item()
    i.values = [True, False, 1, 2, 3.5]

    print(i)

    unittest.main()