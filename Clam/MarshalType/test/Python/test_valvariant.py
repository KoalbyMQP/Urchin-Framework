import pytest
import marshaltype_py
import inspect

print("pytest")
class TestValVariant:

    def test_setup(self):
        valvariant = marshaltype_py.ValVariant(-1)
        methods = [attr for attr in dir(valvariant) if callable(getattr(valvariant, attr))]
        print(methods)


    # -------------------------
    # BASIC EQUALITY
    # -------------------------

    def test_int_equality(self):
        a = marshaltype_py.ValVariant(-1)
        b = marshaltype_py.ValVariant(-1)
        c = marshaltype_py.ValVariant(0)



        assert a == b
        assert not (a == c)

    def test_zero_equality(self):
        a = marshaltype_py.ValVariant(0)
        b = marshaltype_py.ValVariant(0)


        assert a == b

    def test_positive_equality(self):
        a = marshaltype_py.ValVariant(123)
        b = marshaltype_py.ValVariant(123)



        assert a == b

    # -------------------------
    # EDGE CASES
    # -------------------------

    def test_negative_values(self):
        for v in [-1, -2, -100, -(2**31)]:
            a = marshaltype_py.ValVariant(v)
            b = marshaltype_py.ValVariant(v)



            assert a == b

    def test_int32_boundary(self):
        a = marshaltype_py.ValVariant(2**31 - 1)
        b = marshaltype_py.ValVariant(2**31 - 1)


        assert a == b

    # -------------------------
    # SIGNED / UNSIGNED PROBE
    # -------------------------

    def test_signed_unsigned_mismatch_probe(self):
        a = marshaltype_py.ValVariant(-1)
        b = marshaltype_py.ValVariant(4294967295)


        print("\nDEBUG a =", a.get(), "b =", b.get())

        # Diagnostic only
        assert True

    # -------------------------
    # OPERATOR TESTS
    # -------------------------

    def test_eq_operator_works(self):
        a = marshaltype_py.ValVariant(-1)
        b = marshaltype_py.ValVariant(-1)


        assert a == b

    def test_ne_operator(self):
        a = marshaltype_py.ValVariant(-1)
        b = marshaltype_py.ValVariant(1)

        assert a != b

    # -------------------------
    # INTERNAL CONSISTENCY
    # -------------------------

    def test_get_consistency(self):
        a = marshaltype_py.ValVariant(10)
        b = marshaltype_py.ValVariant(10)


        assert a.get() == b.get()

    # -------------------------
    # DEBUG REPRESENTATION
    # -------------------------

    def test_repr_debug(self):
        a = marshaltype_py.ValVariant(-1)
        b = marshaltype_py.ValVariant(-1)

        print("\nrepr a:", repr(a))
        print("repr b:", repr(b))

        assert True

    # -------------------------
    # BINDING INSPECTION
    # -------------------------

    def test_bindings_introspection(self):
        for name, cls in inspect.getmembers(marshaltype_py, inspect.isclass):
            if cls.__module__ == marshaltype_py.__name__:
                print("\nClass:", name)

                methods = [
                    m for m in dir(cls)
                    if callable(getattr(cls, m))
                ]

                for m in methods:
                    print("  └──", m)

        assert True

    # -------------------------
    # FLOAT TESTS
    # -------------------------

    def test_float_equality(self):
        a = marshaltype_py.ValVariant(1.0)
        b = marshaltype_py.ValVariant(1.0)
        c = marshaltype_py.ValVariant(2.0)


        assert a == b
        assert not (a == c)

    def test_float_edge_values(self):
        values = [-1.0, 0.0, 3.14, -2.5, 1e10]

        for v in values:
            a = marshaltype_py.ValVariant(float(v))
            b = marshaltype_py.ValVariant(float(v))


            assert a == b

    # -------------------------
    # BOOL TESTS
    # -------------------------

    def test_bool_equality(self):
        a = marshaltype_py.ValVariant(True)
        b = marshaltype_py.ValVariant(True)
        c = marshaltype_py.ValVariant(False)


        assert a == b
        assert not (a == c)

    def test_bool_int_interaction(self):
        a = marshaltype_py.ValVariant(True)
        b = marshaltype_py.ValVariant(False)
        c = marshaltype_py.ValVariant(1)
        d = marshaltype_py.ValVariant(0)

        e = marshaltype_py.ValVariant(1118872535)
        f = marshaltype_py.ValVariant(88.32)


        print(str(a.get()) + " should be bool")
        assert a.get() is True

        print(str(c.get()) + " should be int")
        assert c.get() == 1

        print(str(b.get()) + " should be bool")
        assert b.get() is False

        print(str(d.get()) + " should be int")
        assert d.get() == 0

        assert a != b
        assert a != c
        assert b != d
        assert c != d
        assert e != f