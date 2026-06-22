
import Clam
import unittest
import inspect



class TestValVariant(unittest.TestCase):

    # -------------------------
    # BASIC EQUALITY
    # -------------------------

    def test_int_equality(self):
        a = Clam.ValVariant(-1)
        b = Clam.ValVariant(-1)
        c = Clam.ValVariant(0)
        self.assertTrue(a == b)
        self.assertFalse(a == c)





    def test_zero_equality(self):
        self.assertEqual(Clam.ValVariant(0), Clam.ValVariant(0))

    def test_positive_equality(self):
        self.assertEqual(Clam.ValVariant(123), Clam.ValVariant(123))


    # -------------------------
    # EDGE CASES
    # -------------------------

    def test_negative_values(self):
        for v in [-1, -2, -100, -2**31]:
            a = Clam.ValVariant(v)
            b = Clam.ValVariant(v)
            self.assertTrue(a == b)

    def test_int32_boundary(self):
        a = Clam.ValVariant(2**31 - 1)
        b = Clam.ValVariant(2**31 - 1)
        self.assertTrue(a == b)


    # -------------------------
    # SIGNED / UNSIGNED PROBE
    # -------------------------

    def test_signed_unsigned_mismatch_probe(self):
        a = Clam.ValVariant(-1)
        b = Clam.ValVariant(4294967295)

        print("\nDEBUG a =", a.get(), "b =", b.get())

        # This should NOT crash; it's diagnostic
        # We do NOT strictly assert equality here because
        # this is where your bug likely lives.
        self.assertTrue(True)


    # -------------------------
    # OPERATOR TESTS
    # -------------------------

    def test_eq_operator_works(self):
        a = Clam.ValVariant(-1)
        b = Clam.ValVariant(-1)
        self.assertTrue(a == b)

    def test_ne_operator(self):
        a = Clam.ValVariant(-1)
        b = Clam.ValVariant(1)
        self.assertTrue(a != b)


    # -------------------------
    # INTERNAL CONSISTENCY
    # -------------------------

    def test_get_consistency(self):
        a = Clam.ValVariant(10)
        b = Clam.ValVariant(10)
        self.assertEqual(a.get(), b.get())


    # -------------------------
    # DEBUG REPRESENTATION
    # -------------------------

    def test_repr_debug(self):
        a = Clam.ValVariant(-1)
        b = Clam.ValVariant(-1)

        print("\nrepr a:", repr(a))
        print("repr b:", repr(b))

        self.assertTrue(True)


    # -------------------------
    # BINDING INSPECTION
    # -------------------------

    def test_bindings_introspection(self):
        for name, cls in inspect.getmembers(Clam, inspect.isclass):
            if cls.__module__ == Clam.__name__:
                print("\nClass:", name)

                methods = [
                    m for m in dir(cls)
                    if callable(getattr(cls, m))
                ]

                for m in methods:
                    print("  └──", m)

        self.assertTrue(True)



    # -------------------------
    # Float Test
    # -------------------------
    def test_float_equality(self):
        a = Clam.ValVariant(float(1.0))
        b = Clam.ValVariant(float(1.0))
        c = Clam.ValVariant(float(2.0))

        self.assertTrue(a == b)
        self.assertFalse(a == c)

    def test_float_edge_values(self):
        values = [-1.0, 0.0, 3.14, -2.5, 1e10]

        for v in values:
            a = Clam.ValVariant(float(v))
            b = Clam.ValVariant(float(v))
            self.assertTrue(a == b)

    # -------------------------
    # Float Test
    # -------------------------
    def test_bool_equality(self):
        a = Clam.ValVariant(True)
        b = Clam.ValVariant(True)
        c = Clam.ValVariant(False)

        self.assertTrue(a == b)
        self.assertFalse(a == c)

    def test_bool_int_interaction(self):
        a = Clam.ValVariant(True)
        b = Clam.ValVariant(False)
        c = Clam.ValVariant(1)
        d = Clam.ValVariant(0)

        e = Clam.ValVariant(1118872535)
        f = Clam.ValVariant(float(88.32))

        print(str(a.get)+"should be bool")
        self.assertEqual(a.get(), True)

        print(str(c.get)+"should be int")
        self.assertEqual(c.get(), 1)

        print(str(b.get)+"should be bool")
        self.assertEqual(b.get(), False)

        print(str(d.get)+"should be int")
        self.assertEqual(d.get(), 0)

        self.assertFalse(a == b)
        self.assertFalse(a == c)

        self.assertFalse(b == d)
        self.assertFalse(c == d)

        self.assertFalse(e == f)





if __name__ == "__main__":
    unittest.main()