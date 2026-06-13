//
// Created by gabri on 6/12/26.
//

#include "MarshalType.h"





class Item : virtual public MarshalType {
public:
    // set joint name max length
    static constexpr size_t MAX_JOINT_LEN = 50;
    static constexpr size_t MAX_VALUES_LEN = 255;


    // set allowed var types
    using Value = std::variant<int, float, bool>;

    std::string joint;
    std::string command;
    std::vector<Value> values;

    Item(const std::string& j,
      const std::string& c,
      const std::vector<Value>& v)
     : joint(j), command(c), values(v){
        if (j.size() > MAX_JOINT_LEN) {
            throw std::runtime_error("joint too long");
        }
        joint = j;
        if (values.size() > MAX_VALUES_LEN) {
            throw std::runtime_error("too many values max is "+std::to_string(MAX_VALUES_LEN));
        }
    }

    std::vector<uint8_t> Press(const Value& value) {

        std::vector<uint8_t> d;

        if (std::holds_alternative<int>(value)) {
            int v = std::get<int>(value);

            d.push_back('I');
            appendBytes(d, &v, sizeof(v));
        }

        else if (std::holds_alternative<float>(value)) {
            float v = std::get<float>(value);

            d.push_back('F');
            appendBytes(d, &v, sizeof(v));
        }

        else if (std::holds_alternative<bool>(value)) {
            bool v = std::get<bool>(value);

            uint8_t b = v ? 1 : 0;

            d.push_back('B');
            appendBytes(d, &b, sizeof(b));
        }

        return d;
    }



    bool Expand(const uint8_t* data,
             size_t size,
             Value& out,
             size_t& consumed)
    {
        if (!data || size < 1)
            return false;

        const uint8_t tag = data[0];
        size_t offset = 1;

        switch (tag)
        {
            // ---------------- INT32 ----------------
            case 'I':
            {
                constexpr size_t len = sizeof(int32_t);

                if (size < offset + len)
                    return false;

                int32_t v;
                std::memcpy(&v, data + offset, len);

                // Python struct defaults to native endian (usually little-endian)
                // If you ever need portability, force little-endian explicitly.
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                v = __builtin_bswap32(v);
#endif

                out = v;
                consumed = offset + len;
                return true;
            }

                // ---------------- FLOAT ----------------
            case 'F':
            {
                constexpr size_t len = sizeof(float);

                if (size < offset + len)
                    return false;

                uint32_t raw = 0;
                std::memcpy(&raw, data + offset, len);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                raw = __builtin_bswap32(raw);
#endif

                float v;
                std::memcpy(&v, &raw, len);

                out = v;
                consumed = offset + len;
                return true;
            }

                // ---------------- BOOL ----------------
            case 'B':
            {
                constexpr size_t len = sizeof(uint8_t);

                if (size < offset + len)
                    return false;

                const uint8_t b = data[offset];

                out = (b != 0);
                consumed = offset + len;
                return true;
            }

            default:
                return false;
        }
    }



    int Marshal(std::vector<uint8_t>& d) {
        try {
            //prep the joint name
            std::vector<uint8_t> temp(MAX_JOINT_LEN, 0);

            std::memcpy(temp.data(),
                        joint.c_str(),
                        std::min(joint.size(), MAX_JOINT_LEN));

            d.insert(d.end(), temp.begin(), temp.end());

            // prep the command len
            appendUint8(d, static_cast<uint8_t>(command.size()));

            // prep the value length
            appendUint8(d, static_cast<uint8_t>(values.size()));

            // prep the command name date
            d.insert(d.end(), reinterpret_cast<const uint8_t*>(command.data()), reinterpret_cast<const uint8_t*>(command.data() + command.size()));

            // press the values
            for (const auto& v : values) {
                std::vector<uint8_t> encoded = Press(v);
                d.insert(d.end(), encoded.begin(), encoded.end());
            }
            return 0; // success
        }
        catch (...) {
            return -2; // generic failure
        }




    }

    int DeMarshal(const std::vector<uint8_t>& d) {
        try {
            size_t offset = 0;

            // ---- joint ----
            if (d.size() < offset + MAX_JOINT_LEN)
                return -1;

            std::string jointStr(
                reinterpret_cast<const char*>(d.data() + offset),
                MAX_JOINT_LEN
            );

            // trim at first null byte
            jointStr = jointStr.c_str();

            offset += MAX_JOINT_LEN;

            // ---- command length ----
            if (offset >= d.size()) return -1;
            uint8_t commandLen = d[offset++];

            // ---- values length ----
            if (offset >= d.size()) return -1;
            uint8_t valuesLen = d[offset++];

            // ---- command ----
            if (d.size() < offset + commandLen)
                return -1;

            std::string commandStr(
                reinterpret_cast<const char*>(d.data() + offset),
                commandLen
            );

            offset += commandLen;

            // ---- values ----
            values.clear();
            values.reserve(valuesLen);

            for (uint8_t i = 0; i < valuesLen; i++) {

                Value v;
                size_t consumed = 0;

                if (!Expand(d.data() + offset,
                             d.size() - offset,
                             v,
                             consumed)) {
                    return -1;
                             }

                values.push_back(v);
                offset += consumed;
            }

            // assign results
            this->joint = jointStr;
            this->command = commandStr;

            return 0;
        }
        catch (...) {
            return -2;
        }
    }



    bool operator==(const Item& other) const {
        return joint == other.joint &&
               command == other.command &&
               values == other.values;
    }

    bool operator!=(const Item& other) const {
        return !(*this == other);
    }
};

std::ostream& operator<<(std::ostream& os, const Item& item) {

    os << "Item{\n";
    os << "  joint: " << item.joint << "\n";
    os << "  command: " << item.command << "\n";
    os << "  values: [";

    for (size_t i = 0; i < item.values.size(); i++) {
        std::visit([&os](const auto& v) {
            os << v;
        }, item.values[i]);

        if (i + 1 < item.values.size())
            os << ", ";
    }

    os << "]\n";
    os << "}";

    return os;
}


namespace py = pybind11;
PYBIND11_MODULE(Clam, m) {
    py::class_<Item>(m, "Item")
        .def(py::init<
        const std::string&,
        const std::string&,
        const std::vector<Item::Value>&
        >())
        .def_readwrite("joint", &Item::joint)
        .def_readwrite("command", &Item::command)
        .def_readwrite("values", &Item::values)
        .def("Marshal", [](Item& self) {
            std::vector<uint8_t> d;
            int err = self.Marshal(d);
            return py::make_tuple(err, d);
        })

        .def("__repr__", [](const Item& item) {
        std::ostringstream oss;
        oss << item;
        return oss.str();})
        .def(py::self == py::self)
        .def(py::self != py::self);

}










