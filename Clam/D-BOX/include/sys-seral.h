//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_SYS_SERAL_H
#define D_BOX_SYS_SERAL_H



#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "../RingBuffer/RingBuffer.h"
#include "../RingBuffer/RingBufferException.hpp"
#include "../RingBuffer/RingBuffer.tpp"

class ISerialPort
{
public:
    virtual ~ISerialPort() = default;

    virtual bool open(const char* device, uint32_t baudRate, uint32_t MStimeout) = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;

    //standard controls
    virtual std::size_t write(std::span<const uint8_t> data, std::size_t size);
    virtual std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size) = 0;

    virtual std::size_t read(std::span<uint8_t> buffer, std::size_t size);
    virtual std::size_t rawread(std::span<uint8_t> buffer, std::size_t size) = 0;

    //advanced controls
    virtual std::size_t peek(std::span<uint8_t> buffer, std::size_t size);
    virtual std::size_t drop(std::size_t size);

    //locator
    virtual std::vector<std::string> find() = 0;
    virtual void flush();
    virtual void flushPeek();
    virtual void flushBuss() = 0;
private:

    RingBuffer<uint8_t,1024> ringBuffer;
protected:
    uint32_t MStimeout;
};

#endif //D_BOX_SYS_SERAL_H