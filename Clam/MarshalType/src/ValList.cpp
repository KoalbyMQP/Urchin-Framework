//
// Created by gabri on 6/13/26.
//
#include "ValList.h"

    int MarshalType::ValVariant::Marshal(std::vector<uint8_t>& d){
        try {
            std::visit([&](auto&& v) {
                using T = std::decay_t<decltype(v)>;

                if constexpr (std::is_same_v<T, int32_t>) {
                    d.push_back('I');
                    appendBytes(d, &v, sizeof(v));
                }
                else if constexpr (std::is_same_v<T, uint32_t>) {
                    d.push_back('U');
                    appendBytes(d, &v, sizeof(v));
                }
                else if constexpr (std::is_same_v<T, float32_t>) {
                    d.push_back('F');
                    appendBytes(d, &v, sizeof(v));
                }
                else if constexpr (std::is_same_v<T, bool32_t>) {
                    uint32_t b = v ? 1 : 0;
                    d.push_back('B');
                    appendBytes(d, &b, sizeof(b));
                }
            }, value);

            return 0;
        }
        catch (...) {
            return -2;
        }
    }

    int MarshalType::ValVariant::DeMarshal(const std::vector<uint8_t>& d) {
        try {
            if (d.empty())
                return -1;

            const uint8_t* data = d.data();
            size_t size = d.size();

            uint8_t tag = data[0];
            size_t offset = 1;

            switch (tag)
            {
                case 'U': // uint32_t
                {
                    constexpr size_t len = sizeof(uint32_t);

                    if (size < offset + len)
                        return -2;

                    uint32_t v;
                    std::memcpy(&v, data + offset, len);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    v = __builtin_bswap32(v);
#endif

                    value = v;
                    return 0;
                }

                case 'I': // int32_t
                {
                    constexpr size_t len = sizeof(int32_t);

                    if (size < offset + len)
                        return -2;

                    int32_t v;
                    std::memcpy(&v, data + offset, len);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    v = __builtin_bswap32(v);
#endif

                    value = v;
                    return 0;
                }

                case 'F':
                {
                    constexpr size_t len = sizeof(float32_t);

                    if (size < offset + len)
                        return -2;

                    uint32_t raw;
                    std::memcpy(&raw, data + offset, len);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    raw = __builtin_bswap32(raw);
#endif

                    float v;
                    std::memcpy(&v, &raw, len);

                    value = v;
                    return 0;
                }

                case 'B':
                {
                    constexpr size_t len = sizeof(bool32_t);

                    if (size < offset + len)
                        return -2;

                    bool32_t v;
                    std::memcpy(&v, data + offset, len);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    v = __builtin_bswap32(v);
#endif

                    value = v ? 1u : 0u;
                    return 0;
                }

                default:
                    return -3;
            }
        }
        catch (...) {
            return -4;
        }
    }





 std::ostream& operator<<(std::ostream& os, const MarshalType::ValVariant& v)
    {
        const auto& var = v.get();

        std::visit([&](const auto& value)
        {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, int32_t>)
            {
                os << "[int32] " << value;
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                os << "[uint32] " << value;
            }
            else if constexpr (std::is_same_v<T, MarshalType::float32_t>)
            {
                os << "[float32] " << value;
            }
            else if constexpr (std::is_same_v<T, MarshalType::bool32_t>)
            {
                os << "[bool32] " << (value.v != 0 ? "true" : "false");
            }
            else
            {
                os << "[unknown] " << value;
            }

        }, var);

        return os;
    }



