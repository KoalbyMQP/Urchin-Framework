//
// Created by gabri on 6/12/26.
//

#include "Item.h"




namespace MarshalType {

    int Item::Marshal(std::vector<uint8_t>& d) {
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
            for (auto& v : values) {
                std::vector<uint8_t> e;
                v.Marshal(e);
                d.insert(d.end(), e.begin(), e.end());
            }
            return 0; // success
        }
        catch (...) {
            return -2; // generic failure
        }




    }

    int Item::DeMarshal(const std::vector<uint8_t>& d) {
        try {
            size_t offset = 0;

            // ---- joint ----
            if (d.size() < offset + Item::MAX_JOINT_LEN)
                return -1;

            std::string jointStr(
                reinterpret_cast<const char*>(d.data() + offset),
                Item::MAX_JOINT_LEN
            );

            // trim at first null byte
            jointStr = jointStr.c_str();

            offset += Item::MAX_JOINT_LEN;

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
            this->values.clear();
            this->values.reserve(valuesLen);

            for (uint8_t i = 0; i < valuesLen; i++) {

                ValVariant v;
                size_t consumed = 0;

                std::vector<uint8_t> buff(
                    d.begin() + offset,
                    d.begin() + offset + consumed
                );
                if (!v.DeMarshal(buff))
                    return -1;

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




};

std::ostream& operator<<(std::ostream& os, const MarshalType::Item& item) {

    os << "Item{\n";
    os << "  joint: " << item.joint << "\n";
    os << "  command: " << item.command << "\n";
    os << "  values: [";

    for (size_t i = 0; i < item.values.size(); i++) {

        std::visit([&os](const auto& v) {
            using T = std::decay_t<decltype(v)>;

            if constexpr (std::is_same_v<T, MarshalType::bool32_t>) {
                os << (bool)v;
            }
            else {
                os << v;
            }

        }, item.values[i].get());

        if (i + 1 < item.values.size())
            os << ", ";
    }

    os << "]\n";
    os << "}";

    return os;
}



