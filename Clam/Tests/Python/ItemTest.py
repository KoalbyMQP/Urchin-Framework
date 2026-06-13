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
        self.assertTrue(item == item)


if __name__ == "__main__":


    unittest.main()