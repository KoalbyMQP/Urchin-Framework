//
// Created by gabri on 6/22/2026.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <MarshalType.h>
#include <ValList.h>
#include <Item.h>

#include <pybind11/stl.h>
#include <cstdint>


namespace py = pybind11;
using namespace MarshalType;

// Turns one Python value into a ValVariant: an existing ValVariant is kept
// as-is, bool -> bool, int -> int32 (or uint32 if it only fits unsigned),
// float -> float32. Anything else is rejected.
static ValVariant ToValVariant(py::handle h) {
    if (py::isinstance<ValVariant>(h)) {
        return py::cast<ValVariant>(h);
    }
    if (py::isinstance<py::bool_>(h)) {   // check bool before int: True is an int in Python
        return ValVariant(py::cast<bool>(h));
    }
    if (py::isinstance<py::int_>(h)) {
        long long v = py::cast<long long>(h);
        if (v >= INT32_MIN && v <= INT32_MAX) {
            return ValVariant(static_cast<int32_t>(v));
        }
        if (v >= 0 && v <= static_cast<long long>(UINT32_MAX)) {
            return ValVariant(static_cast<uint32_t>(v));
        }
        throw py::value_error("integer does not fit in 32 bits");
    }
    if (py::isinstance<py::float_>(h)) {
        return ValVariant(py::cast<float>(h));
    }
    throw py::type_error("values must be bool, int, float or ValVariant");
}

static std::vector<ValVariant> ToValVariants(const py::iterable& items) {
    std::vector<ValVariant> out;
    for (auto h : items) {
        out.push_back(ToValVariant(h));
    }
    return out;
}
PYBIND11_MODULE(marshaltype_py, m) {
    py::class_<ValVariant>(m, "ValVariant")
        .def(py::init<>())
        .def(py::init<bool>())
        .def(py::init<int32_t>())
        .def(py::init<uint32_t>())
        .def(py::init<float32_t>())


        .def("set",
            [](ValVariant& self, py::object obj)
            {
                if (py::isinstance<py::bool_>(obj))
                {
                    self.set(obj.cast<bool>());
                }
                else if (py::isinstance<py::float_>(obj))
                {
                    self.set(obj.cast<float32_t>());
                }
                else if (py::isinstance<py::int_>(obj))
                {
                    long long v = obj.cast<long long>();

                    if (v < 0)
                        self.set(static_cast<int32_t>(v));
                    else
                        self.set(static_cast<uint32_t>(v));
                }
                else
                {
                    throw std::runtime_error("Unsupported type");
                }
            })

        .def("get", [](const ValVariant& v) -> py::object {
            return std::visit([](auto&& val) -> py::object {
                using T = std::decay_t<decltype(val)>;

                if constexpr (std::is_same_v<T, bool32_t>) {
                    return py::bool_(static_cast<bool>(val));
                }
                else if constexpr (std::is_same_v<T, int32_t>) {
                    return py::int_(val);
                }
                else if constexpr (std::is_same_v<T, uint32_t>) {
                    return py::int_(val);
                }
                else if constexpr (std::is_same_v<T, float32_t>) {
                    return py::float_(val);
                }
                else {
                    return py::none();
                }
            }, v.get());
            })

        .def("marshal",[](ValVariant& self){
                std::vector<uint8_t> d;
            int err = self.Marshal(d);
            return py::make_tuple(err, d);
            })

        .def("demarshal",[](ValVariant& self, const std::vector<uint8_t>& d){
            return self.DeMarshal(d);
            })
        .def("__repr__", [](const ValVariant& Val) {
            std::ostringstream oss;
            oss << Val;
            return oss.str();})
        // Compare with another ValVariant or with a plain Python value
        // (ValVariant(1) == 1), converted by the same rules as Item values.
        .def("__eq__", [](const ValVariant& a, py::object b) {
            try { return a == ToValVariant(b); }
            catch (const py::error_already_set&) { throw; }
            catch (const std::exception&) { return false; }
            })
        .def("__ne__", [](const ValVariant& a, py::object b) {
            try { return !(a == ToValVariant(b)); }
            catch (const py::error_already_set&) { throw; }
            catch (const std::exception&) { return true; }
            });

    py::class_<Item>(m, "Item")
        // values may be plain Python numbers/bools or ValVariants:
        // Item("left_knee", "Move", [30.0, 1000])
        .def(py::init([](const std::string& joint, const std::string& command, py::iterable values) {
            return Item(joint, command, ToValVariants(values));
        }), py::arg("joint"), py::arg("command"), py::arg("values"))
        .def(py::init<>())

        .def_readwrite("joint", &Item::joint)
        .def_readwrite("command", &Item::command)



    .def_property(
    "values",

    [](const Item& self) {
        return self.values;
    },

    [](Item& self, py::iterable lst) {
        self.values = ToValVariants(lst);
    }
)


        .def("Marshal", [](Item& self) {
                std::vector<uint8_t> d;
            int err = self.Marshal(d);
            return py::make_tuple(err, d);
        })
        .def("DeMarshal",[](Item& self, const std::vector<uint8_t>& d) {
            return self.DeMarshal(d);
        })

        .def("__repr__", [](const Item& item) {
        std::ostringstream oss;
        oss << item;
        return oss.str();})
    .def("__eq__", [](const Item& a, const Item& b) {
        return a == b;
        })
    .def("__ne__", [](const Item& a, const Item& b) {
    return !(a == b);
        });
}













