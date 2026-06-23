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

        Item(const std::string& j,
          const std::string& c,
          const std::vector<ValVariant>& v)
         : joint(j), command(c), values(v){
            if (j.size() > MAX_JOINT_LEN) {
                throw std::runtime_error("joint too long");
            }
            joint = j;
            if (values.size() > MAX_VALUES_LEN) {
                throw std::runtime_error("too many values max is "+std::to_string(MAX_VALUES_LEN));
            }
        }


        Item() = default;

        int Marshal(std::vector<uint8_t>& d);
        int DeMarshal(const std::vector<uint8_t>& d);

    };
}
#endif //MARSHALTYPE_ITEM_H