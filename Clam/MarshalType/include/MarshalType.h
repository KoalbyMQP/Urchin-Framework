//
// Created by gabri on 6/12/26.
//

#ifndef CLAM_MARSHALTYPE_H
#define CLAM_MARSHALTYPE_H
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <cstddef>

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace MarshalType {
struct bool32_t {
    uint32_t v;

    constexpr bool32_t() noexcept : v(0) {}
    constexpr bool32_t(bool b) noexcept : v(b ? 1u : 0u) {}

    constexpr explicit operator bool() const noexcept {
        return v != 0;
    }

    constexpr bool32_t& operator=(bool b) noexcept {
        v = b ? 1u : 0u;
        return *this;
    }

    constexpr bool operator==(const bool32_t& other) const noexcept {
        return v == other.v;
    }

    constexpr bool operator!=(const bool32_t& other) const noexcept {
        return v != other.v;
    }

    constexpr bool operator==(bool b) const noexcept {
        return (v != 0) == b;
    }
};
using float32_t = float;


    class MarshalType {
    public:
        /*
         * @brief This defines the interface for any type to be set over
         the serial or a network that converts a complicated data type to
         an int8_t array
         *
         * @details If you are using any field types that are not
         primitives, you should have that type inherit from MarshalType.
         *
         * @param d A pointer to a buffer to write to
         *
         * @return The error state of the function
         * @retval 0 Success
         * @retval 1 an error occurred
         * @retval 2 memory allocation error occurred
         */
        virtual int Marshal(std::vector<uint8_t>& d) = 0;

        /*
         * @brief This defines the interface for any type to be set over
         the serial or a network that converts an int8_t array to a
         complicated data type.
         *
         * @details If you are using any field types that are not
         primitives, you should have that type inherit from MarshalType.
         *
         * @param d A pointer to a buffer to read from
         *
         * @return The error state of the function
         * @retval 0 Success
         * @retval 1 an error occurred
         * @retval 2 memory allocation error occurred
         */
        virtual int DeMarshal(const std::vector<uint8_t>& d) = 0;

        /*
         * @brief This defines the destructor for the class and should
         also destruct all its fields
         */
        virtual ~MarshalType() {}

        /**
        * @brief Appends raw bytes to a byte buffer.
        *
        * Copies `size` bytes from the memory pointed to by `data`
        * and appends them to the end of `d`.
        *
        * @warning
        * - `data` must point to at least `size` valid bytes.
        * - This is a raw memory copy; no type or bounds checking is performed.
        *
        * @param d      Destination byte buffer.
        * @param data   Pointer to source memory block.
        * @param size   Number of bytes to append.
        */
        inline void appendBytes(std::vector<uint8_t>& d, const void* data, size_t size) {
            if (size == 0 || data == nullptr) return;

            const auto* bytes = static_cast<const uint8_t*>(data);
            d.insert(d.end(), bytes, bytes + size);
        }

        /**
         * @brief Appends a single byte to the buffer.
         *
         * @param d      Destination byte buffer.
         * @param value  Byte value to append.
         */
        inline void appendUint8(std::vector<uint8_t>& d, uint8_t value) {
            d.push_back(value);
        }

    };
}

#endif //CLAM_MARSHALTYPE_H