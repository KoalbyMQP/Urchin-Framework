//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_RINGBUFFEREXCEPTION_HPP_H
#define D_BOX_RINGBUFFEREXCEPTION_HPP_H
#include <stdexcept>

namespace RingBuffer {
    class RingBufferException : public std::runtime_error
    {
    public:
        explicit RingBufferException(const std::string& msg)
            : std::runtime_error(msg) {}
    };


    // Tried to read more elements than are available
    class RingBufferUnderflowException : public RingBufferException
    {
    public:
        explicit RingBufferUnderflowException(size_t attempted, size_t available, size_t actuallyRead)
            : RingBufferException("RingBuffer underflow"),
              attempted(attempted),
              available(available),
              actuallyRead(actuallyRead)
        {}

        size_t attempted;
        size_t available;
        size_t actuallyRead;
    };


    // Tried to read more elements than the ring can store
    class RingBufferBoundException : public RingBufferException
    {
    public:
        RingBufferBoundException()
            : RingBufferException("RingBuffer BoundException: number of values requested larger then ring") {}
    };

    // Output span is too small for requested read
    class RingBufferSpanTooSmallException : public RingBufferException
    {
    public:
        RingBufferSpanTooSmallException()
            : RingBufferException("RingBuffer error: output span too small for requested count") {}
    };
}
#endif //D_BOX_RINGBUFFEREXCEPTION_HPP_H