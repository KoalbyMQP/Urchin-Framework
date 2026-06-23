//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_LINUX_SYS_SERAL_H
#define D_BOX_LINUX_SYS_SERAL_H

#include "../sys-seral.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <span>
#include <string_view>
#include <cstdint>
#include <cstddef>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <span>
#include <string>
#include <system_error>
#include <termios.h>
#include <unistd.h>
#include <vector>

class LinuxSerialPort : public ISerialPort
{
public:
    LinuxSerialPort() = default;
    ~LinuxSerialPort();

    std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size);
    bool open(const char* device, uint32_t baudRate);
    void close();
    bool isOpen() const;
    std::size_t rawread(std::span<uint8_t> buffer, std::size_t size);
    std::vector<std::string> find();
    void flushBuss();

private:
    int m_fd = -1;

    static bool configurePort(int fd, uint32_t baudRate);
    static speed_t toBaud(uint32_t baudRate);
};

#endif //D_BOX_LINUX_SYS_SERAL_H