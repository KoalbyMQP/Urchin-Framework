//
// Created by gabri on 6/22/2026.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <MarshalType.h>
#include <ValList.h>


#include <pybind11/stl.h>


namespace py = pybind11;
using namespace MarshalType;
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
        .def("__eq__", [](const ValVariant& a, const ValVariant& b) {
            return a == b;
            })
        .def("__ne__", [](const ValVariant& a, const ValVariant& b) {
        return !(a == b);
            });

    /*py::class_<Item>(m, "Item")
        .def(py::init<
        const std::string&,
        const std::string&,
        const std::vector<Item::Value>&
        >())
        .def(py::init<>())

        .def_readwrite("joint", &Item::joint)
        .def_readwrite("command", &Item::command)



    .def_property(
    "values",

    [](const Item& self) {
        return self.values;
    },

    [](Item& self, py::list lst) {

        self.values.clear();
        self.values.reserve(py::len(lst));

        for (auto obj : lst) {

            py::handle h = obj;

            // IMPORTANT: bool check FIRST using exact type check
            if (py::isinstance<py::bool_>(h)) {
                self.values.emplace_back(py::cast<bool>(h));
            }

            // THEN int
            else if (py::isinstance<py::int_>(h)) {
                self.values.emplace_back(py::cast<int>(h));
            }

            // THEN float
            else if (py::isinstance<py::float_>(h)) {
                self.values.emplace_back(py::cast<float>(h));
            }

            else {
                throw std::runtime_error("values must be bool, int, or float");
            }
        }
    }
)


        .def("Marshal", [](Item& self) {
            std::vector<uint8_t> d;
            int err = self.Marshal(d);
            return py::make_tuple(err, d);
        })
        .def("DeMarshal",
            [](Item& self, const std::vector<uint8_t>& d) {
            return self.DeMarshal(d);
        })

        .def("__repr__", [](const Item& item) {
        std::ostringstream oss;
        oss << item;
        return oss.str();})
        .def(py::self == py::self)
        .def(py::self != py::self);*/
}













