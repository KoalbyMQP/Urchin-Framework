//
// Created by gabri on 6/18/2026.
//

#include <Transport-Interface/IPort.h>
#include <RingBuffer/RingBufferException.hpp>
namespace DBOXProtocol {
    std::size_t IPort::write(std::span<const uint8_t> data, std::size_t size) {
        return rawwrite(data, size);
    }


    std::size_t IPort::read(std::span<uint8_t> buffer, std::size_t size) {
        std::size_t total = 0;

        // 1. take from ring buffer first
        std::size_t fromBuffer = std::min(size, ringBuffer.size());
        if (fromBuffer > 0)
        {
            try {
                total += ringBuffer.pop(buffer, fromBuffer);
            }
            catch (const RingBuffer::RingBufferUnderflowException& e) {
            }

        }

        // 2. fill remainder from raw device
        if (total < size)
        {
            total += rawread(buffer.subspan(total), size - total);
        }

        return total;
    }


    std::size_t IPort::peek(std::span<uint8_t> buffer, std::size_t size) {
        // 1. ensure ring buffer has enough data
        if (ringBuffer.size() < size)
        {
            std::size_t missing = size - ringBuffer.size();

            std::vector<uint8_t> tmp(missing);

            std::size_t readNow = rawread(tmp, missing);

            ringBuffer.push(std::span<const uint8_t>(tmp.data(), readNow));
        }

        // 2. now safe: buffer has enough data (or as much as possible)
        return ringBuffer.peek(buffer, size);
    }

    std::size_t IPort::drop(std::size_t size) {
        std::size_t dropped = 0;

        // 1. drop buffered data first
        std::size_t fromBuffer = std::min(size, ringBuffer.size());
        if (fromBuffer > 0)
        {
            std::array<uint8_t, 256> tmp{};
            dropped += ringBuffer.pop(std::span<uint8_t>(tmp.data(), fromBuffer), fromBuffer);
        }

        // 2. drop remaining directly from raw stream
        if (dropped < size)
        {
            std::vector<uint8_t> sink(size - dropped);
            dropped += rawread(sink, size - dropped);
        }

        return dropped;
    }

    void IPort::flushPeek() {
        ringBuffer = RingBuffer::RingBuffer<uint8_t, 1024>{};
    }

    void  IPort::flush() {
        flushPeek();
        flushBuss();
    }
}