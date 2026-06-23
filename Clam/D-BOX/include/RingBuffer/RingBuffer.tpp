//
// Created by gabri on 6/18/2026.
//

//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_RINGBUFFER_TPP
#define D_BOX_RINGBUFFER_TPP

#include <algorithm>
#include "RingBuffer.h"
#include "RingBufferException.hpp"


template<typename T, size_t Capacity>
size_t RingBuffer<T, Capacity>::pop(std::span<T> out, size_t n)
{
    size_t available = count;
    size_t toRead = std::min(n, available);

    if (out.size() < toRead)
        throw RingBufferSpanTooSmallException();

    // copy everything available (or requested, whichever is smaller)
    for (size_t i = 0; i < toRead; ++i)
    {
        out[i] = buffer[head];
        head = (head + 1) % Capacity;
    }

    count -= toRead;

    // IMPORTANT: only signal failure AFTER successful copy
    if (n > available)
    {
        throw RingBufferUnderflowException(n, available, toRead);
    }

    return toRead;
}

template<typename T, size_t Capacity>
size_t RingBuffer<T, Capacity>::peek(std::span<T> out, size_t n) const
{
    size_t available = count;
    size_t toRead = std::min(n, available);

    if (out.size() < toRead)
        throw RingBufferSpanTooSmallException();

    size_t tempHead = head;

    for (size_t i = 0; i < toRead; ++i)
    {
        out[i] = buffer[tempHead];
        tempHead = (tempHead + 1) % Capacity;
    }

    // Peek must NOT modify state, so no changes to head/count/tail

    // Optional behavior choice:
    // If you want peek to behave like pop (strict):
    if (n > available)
        throw RingBufferUnderflowException(n, available, toRead);

    return toRead;
}

#endif // D_BOX_RINGBUFFER_TPP