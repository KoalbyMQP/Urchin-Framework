//
// Created by gabri on 6/22/2026.
//

#ifndef MARSHALTYPE_ITEM_H
#define MARSHALTYPE_ITEM_H

#include <MarshalType.h>
#include <ValList.h>

namespace MarshalType {
    class Item : virtual public MarshalType {
    public:
        // set joint name max length
        static constexpr size_t MAX_JOINT_LEN = 50;
        static constexpr size_t MAX_VALUES_LEN = 255;


        // set allowed var types


        std::string joint;
        std::string command;
        std::vector<ValVariant> values;

        //CORE CONSTRUCTOR
        Item(const std::string& j,
             const std::string& c,
             const std::vector<ValVariant>& v)
            : joint(j), command(c), values(v)
        {
            if (j.size() > MAX_JOINT_LEN) {
                throw std::runtime_error("joint too long");
            }

            if (values.size() > MAX_VALUES_LEN) {
                throw std::runtime_error(
                    "too many values max is " + std::to_string(MAX_VALUES_LEN)
                );
            }
        }

        //VARIADIC CONSTRUCTOR
        template <
            typename... Args,
            std::enable_if_t<
                (!std::is_same_v<std::decay_t<Args>, std::vector<ValVariant>> && ...),
                int
            > = 0
        >
        Item(const std::string& j,
             const std::string& c,
             Args&&... args)
            : Item(j, c, MakeValues(std::forward<Args>(args)...))
        {}


        //DEFAULT CONSTRUCTOR
        Item() = default;


        int Marshal(std::vector<uint8_t>& d);
        int DeMarshal(const std::vector<uint8_t>& d);


        bool operator==(const Item& other) const {
            return this->joint == other.joint &&
                   this->command == other.command &&
                   this->values == other.values;
        }

        bool operator!=(const Item& other) const {
            return !(*this == other);
        }

        private:
        static std::vector<ValVariant> MakeValues()
        {
            return {};
        }

        template <typename... Args>
        static std::vector<ValVariant> MakeValues(Args&&... args)
        {
            std::vector<ValVariant> v;
            v.reserve(sizeof...(Args));

            (v.emplace_back(ValVariant(std::forward<Args>(args))), ...);

            return v;
        }
    };
    std::ostream& operator<<(std::ostream& os, const Item& item);

}
#endif //MARSHALTYPE_ITEM_H