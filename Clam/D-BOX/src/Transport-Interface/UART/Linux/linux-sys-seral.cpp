#include "../../linux-sys-seral.h"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <termios.h>
#include <unistd.h>
#include <vector>
namespace DBOXProtocol {
    namespace fs = std::filesystem;

    LinuxSerialPort::~LinuxSerialPort()
    {
        close();
    }

    speed_t LinuxSerialPort::toBaud(uint32_t baudRate)
    {
        switch (baudRate)
        {
            case 50: return B50;
            case 75: return B75;
            case 110: return B110;
            case 134: return B134;
            case 150: return B150;
            case 200: return B200;
            case 300: return B300;
            case 600: return B600;
            case 1200: return B1200;
            case 1800: return B1800;
            case 2400: return B2400;
            case 4800: return B4800;
            case 9600: return B9600;
            case 19200: return B19200;
            case 38400: return B38400;
            case 57600: return B57600;
            case 115200: return B115200;
            case 230400: return B230400;
#ifdef B460800
            case 460800: return B460800;
#endif
#ifdef B500000
            case 500000: return B500000;
#endif
#ifdef B576000
            case 576000: return B576000;
#endif
#ifdef B921600
            case 921600: return B921600;
#endif
#ifdef B1000000
            case 1000000: return B1000000;
#endif
#ifdef B1152000
            case 1152000: return B1152000;
#endif
#ifdef B1500000
            case 1500000: return B1500000;
#endif
#ifdef B2000000
            case 2000000: return B2000000;
#endif
#ifdef B2500000
            case 2500000: return B2500000;
#endif
#ifdef B3000000
            case 3000000: return B3000000;
#endif
#ifdef B3500000
            case 3500000: return B3500000;
#endif
#ifdef B4000000
            case 4000000: return B4000000;
#endif
            default:
                return 0;
        }
    }

    bool LinuxSerialPort::configurePort(int fd, uint32_t baudRate)
    {
        const speed_t speed = toBaud(baudRate);
        if (speed == 0)
            return false;

        termios tty{};
        if (::tcgetattr(fd, &tty) != 0)
            return false;

        // Put port in raw mode first
        ::cfmakeraw(&tty);

        // 8N1
        tty.c_cflag &= ~PARENB;   // no parity
        tty.c_cflag &= ~CSTOPB;   // 1 stop bit
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;       // 8 data bits

        // Enable receiver, ignore modem control lines
        tty.c_cflag |= (CREAD | CLOCAL);

        // No hardware flow control
#ifdef CRTSCTS
        tty.c_cflag &= ~CRTSCTS;
#endif

        // No software flow control
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);

        // Blocking read semantics:
        // - wait until at least 1 byte arrives
        // - no inter-byte timeout
        tty.c_cc[VMIN]  = 1;
        tty.c_cc[VTIME] = 0;

        if (::cfsetispeed(&tty, speed) != 0)
            return false;

        if (::cfsetospeed(&tty, speed) != 0)
            return false;

        if (::tcsetattr(fd, TCSANOW, &tty) != 0)
            return false;

        // Drop any stale bytes present when the port is opened/configured
        ::tcflush(fd, TCIOFLUSH);

        return true;
    }

    bool LinuxSerialPort::open(const char* device, uint32_t baudRate)
    {
        close();

        if (device == nullptr || *device == '\0')
            return false;

        // Blocking descriptor:
        // - O_RDWR   : read/write
        // - O_NOCTTY : don't make this process the controlling terminal
        int fd = ::open(device, O_RDWR | O_NOCTTY);
        if (fd < 0)
            return false;

        if (!configurePort(fd, baudRate))
        {
            ::close(fd);
            return false;
        }

        m_fd = fd;
        return true;
    }

    void LinuxSerialPort::close()
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    bool LinuxSerialPort::isOpen() const
    {
        return m_fd >= 0;
    }

    std::size_t LinuxSerialPort::rawwrite(std::span<const uint8_t> data, std::size_t size)
    {
        if (!isOpen() || data.empty() || size == 0)
            return 0;

        size = std::min(size, data.size());

        std::size_t totalWritten = 0;
        while (totalWritten < size)
        {
            const ssize_t written =
                ::write(m_fd, data.data() + totalWritten, size - totalWritten);

            if (written > 0)
            {
                totalWritten += static_cast<std::size_t>(written);
                continue;
            }

            if (written < 0 && errno == EINTR)
                continue;

            // Any other failure (or unexpected 0) stops the write
            break;
        }

        return totalWritten;
    }

    std::size_t LinuxSerialPort::rawread(std::span<uint8_t> buffer, std::size_t size)
    {
        if (!isOpen() || buffer.empty() || size == 0)
            return 0;

        size = std::min(size, buffer.size());

        for (;;)
        {
            const ssize_t bytesRead = ::read(m_fd, buffer.data(), size);

            if (bytesRead > 0)
                return static_cast<std::size_t>(bytesRead);

            if (bytesRead == 0)
                return 0;

            if (errno == EINTR)
                continue;

            // Any other read error
            return 0;
        }
    }

    void LinuxSerialPort::flushBus()
    {
        if (!isOpen())
            return;

        // Wait until all queued output has been transmitted
        while (::tcdrain(m_fd) != 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
    }

    void LinuxSerialPort::clearInputBuffer()
    {
        if (!isOpen())
            return;

        ::tcflush(m_fd, TCIFLUSH);
    }

    void LinuxSerialPort::clearOutputBuffer()
    {
        if (!isOpen())
            return;

        ::tcflush(m_fd, TCOFLUSH);
    }

    std::vector<std::string> LinuxSerialPort::find()
    {
        std::vector<std::string> ports;
        std::error_code ec;

        auto isSerialDeviceName = [](const std::string& name) -> bool
        {
            return
                name.rfind("ttyS",   0) == 0 ||   // PC UARTs
                name.rfind("ttyUSB", 0) == 0 ||   // USB serial adapters
                name.rfind("ttyACM", 0) == 0 ||   // CDC ACM / Arduino / modems
                name.rfind("ttyAMA", 0) == 0 ||   // ARM UARTs
                name.rfind("rfcomm", 0) == 0 ||   // Bluetooth serial
                name.rfind("ttyTHS", 0) == 0 ||   // NVIDIA Jetson
                name.rfind("ttyPS",  0) == 0 ||   // Xilinx / Zynq
                name.rfind("ttyXRUSB", 0) == 0;   // Exar / XR USB serial
        };

        // Scan /dev for common serial device nodes
        for (const auto& entry : fs::directory_iterator("/dev", ec))
        {
            if (ec)
                break;

            const std::string name = entry.path().filename().string();
            if (!isSerialDeviceName(name))
                continue;

            std::error_code sec;
            if (fs::is_character_file(entry.path(), sec))
                ports.push_back(entry.path().string());
        }

        // Add stable symlinks if present (often preferable to /dev/ttyUSBx)
        const fs::path byId("/dev/serial/by-id");
        if (fs::exists(byId, ec) && fs::is_directory(byId, ec))
        {
            for (const auto& entry : fs::directory_iterator(byId, ec))
            {
                if (ec)
                    break;

                ports.push_back(entry.path().string());
            }
        }

        std::sort(ports.begin(), ports.end());
        ports.erase(std::unique(ports.begin(), ports.end()), ports.end());

        return ports;
    }
}