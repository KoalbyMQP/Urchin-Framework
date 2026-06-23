//
// Created by gabri on 6/13/26.
//

#ifndef CLAM_VALLIST_H
#define CLAM_VALLIST_H
#include "MarshalType.h"
namespace MarshalType {
    class ValVariant : virtual public MarshalType {
    public:
        static constexpr size_t PACKET_SIZE = sizeof(int32_t) ;
        using VariantType = std::variant<int32_t, uint32_t, float32_t, bool32_t>;

        int Marshal(std::vector<uint8_t>& d);
        int DeMarshal(const std::vector<uint8_t>& d);

        const VariantType& get() const { return value; }

        template <typename T>
        void set(T v) {
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
                if constexpr (std::is_signed_v<T>)
                    value = static_cast<int32_t>(v);
                else
                    value = static_cast<uint32_t>(v);
            }
            else if constexpr (std::is_floating_point_v<T>) {
                value = static_cast<float32_t>(v);
            }
            else if constexpr (std::is_same_v<T, bool>) {
                value = static_cast<bool32_t>(v);
            }
        }

        ValVariant() : value(int32_t(0)) {}

        template <typename T,
                  typename = std::enable_if_t<
                      std::is_integral_v<T> && !std::is_same_v<T, bool>>>
        ValVariant(T v) {
            if constexpr (std::is_signed_v<T>)
                value = static_cast<int32_t>(v);
            else
                value = static_cast<uint32_t>(v);
        }


        ValVariant(float v): value(static_cast<float32_t>(v)) {}
        ValVariant(bool v): value(static_cast<bool32_t>(v)) {}

        bool operator==(const ValVariant& other) const {
            return std::visit(
                [](auto&& a, auto&& b) -> bool {
                    using A = std::decay_t<decltype(a)>;
                    using B = std::decay_t<decltype(b)>;

                    if constexpr (!std::is_same_v<A, B>) {
                        return false;
                    } else {
                        return a == b;
                    }
                },
                value, other.value
            );
        }

        bool operator!=(const ValVariant& other) const {
            return !(*this == other);
        }

    private:
        VariantType value;

    };


}
std::ostream& operator<<(std::ostream& os, const MarshalType::ValVariant& v);
#endif //CLAM_VALLIST_H