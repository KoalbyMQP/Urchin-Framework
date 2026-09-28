//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_RINGBUFFER_H
#define D_BOX_RINGBUFFER_H


#include <vector>
#include <span>
#include <cstddef>
#include <stdexcept>

namespace RingBuffer {
    template<typename T,size_t Capacity>
    class RingBuffer
    {
    public:
        RingBuffer() = default;

        bool empty() const noexcept { return count == 0; }
        bool full()  const noexcept { return count == Capacity; }

        size_t size() const noexcept { return count; }
        constexpr size_t capacity() const noexcept { return Capacity; }


        size_t push(std::span<const T> in)
        {
            size_t toWrite = std::min(in.size(), Capacity - count);
            for (size_t i = 0; i < toWrite; ++i)
            {
                buffer[tail] = in[i];
                tail = (tail + 1) % Capacity;
            }
            count += toWrite;
            return toWrite;
        }

        size_t pop(std::span<T> out, size_t count);

        // throws if count exceeds available data
        size_t peek(std::span<T> out, size_t count) const;

    private:
        std::array<T, Capacity> buffer{};
        size_t head = 0;
        size_t tail = 0;
        size_t count = 0;
    };
}
#endif //D_BOX_RINGBUFFER_H