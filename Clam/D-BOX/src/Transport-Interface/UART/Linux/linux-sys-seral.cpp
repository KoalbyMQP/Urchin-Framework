//
// Created by gabri on 6/18/2026.
// Updated 2026-11-03 to the current IPort / UARTSerialPort<UartDiscovery>
// interface. toBaud() and configurePort() are Gabe's originals (with read
// timeouts moved to poll() and HUPCL cleared).
//

#include <Transport-Interface/UART/Linux/linux-sys-seral.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <span>
#include <string>
#include <sys/ioctl.h>
#include <system_error>
#include <termios.h>
#include <thread>
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

        // Non-blocking reads at the termios level; rawread() waits with
        // poll() for at most the configured timeout instead. (VMIN=1 made
        // read() block forever, so receive()/discovery timeouts never fired.)
        tty.c_cc[VMIN]  = 0;
        tty.c_cc[VTIME] = 0;

        // Don't drop DTR/RTS on close (that would reset an ESP32 again).
        tty.c_cflag &= ~HUPCL;

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

    bool LinuxSerialPort::open()
    {
        if (this->Discovery == nullptr || this->Discovery->config == nullptr)
            return false;
        auto* cfg = this->Discovery->config;
        return openraw(cfg->Port.c_str(), cfg->baud, cfg->timeout);
    }

    bool LinuxSerialPort::openraw(const char* device, uint32_t baudRate, uint32_t timeoutMs)
    {
        close();

        if (device == nullptr || *device == '\0')
            return false;

        // O_NOCTTY: don't become the controlling terminal.
        // O_NONBLOCK: don't hang in open() waiting for carrier detect.
        int fd = ::open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
            return false;

        if (!configurePort(fd, baudRate))
        {
            ::close(fd);
            return false;
        }

        // Back to blocking mode; rawread() bounds every wait with poll().
        int flags = ::fcntl(fd, F_GETFL);
        if (flags >= 0)
            ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        // Leave DTR/RTS deasserted (EN/BOOT on ESP32 boards). Not every
        // device supports modem lines (e.g. ptys), so errors are ignored.
        int lines = TIOCM_DTR | TIOCM_RTS;
        (void)::ioctl(fd, TIOCMBIC, &lines);

        m_fd = fd;
        m_timeoutMs = timeoutMs;
        this->flushPeek();
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

        // Wait at most m_timeoutMs for data, then report "nothing yet" (0).
        pollfd pfd{m_fd, POLLIN, 0};
        for (;;)
        {
            const int ready = ::poll(&pfd, 1, static_cast<int>(m_timeoutMs));
            if (ready < 0 && errno == EINTR)
                continue;
            if (ready <= 0 || !(pfd.revents & POLLIN))
                return 0;
            break;
        }

        for (;;)
        {
            const ssize_t bytesRead = ::read(m_fd, buffer.data(), size);

            if (bytesRead > 0)
                return static_cast<std::size_t>(bytesRead);

            if (bytesRead < 0 && errno == EINTR)
                continue;

            return 0;
        }
    }

    void LinuxSerialPort::flushBuss()
    {
        if (!isOpen())
            return;

        ::tcflush(m_fd, TCIOFLUSH);
    }

    void LinuxSerialPort::wait(uint32_t time_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
    }

    // Lists serial devices worth trying:
    //  - USB serial (ttyUSB*, ttyACM*, ttyXRUSB*), on-board UARTs (ttyAMA*,
    //    ttyTHS*, ttyPS*), Bluetooth serial (rfcomm*);
    //  - ttyS* only when the kernel found real hardware behind it
    //    (/sys/class/tty/<name>/type != 0). Most PCs expose 4-32 empty ttyS
    //    nodes; trying each would add ~1.5 s per port to discovery.
    // Symlinks (/dev/serial0, /dev/serial/by-id/...) are not listed: they
    // point at these same devices, and listing both would try a port twice.
    std::vector<std::string> LinuxSerialPort::find()
    {
        std::vector<std::string> ports;
        std::error_code ec;

        auto startsWith = [](const std::string& s, const char* p) { return s.rfind(p, 0) == 0; };

        auto isRealTtyS = [](const std::string& name) -> bool
        {
            std::ifstream typeFile("/sys/class/tty/" + name + "/type");
            int type = 0;
            return typeFile >> type && type != 0;
        };

        for (const auto& entry : fs::directory_iterator("/dev", ec))
        {
            if (ec)
                break;

            const std::string name = entry.path().filename().string();
            const bool candidate =
                startsWith(name, "ttyUSB") || startsWith(name, "ttyACM") ||
                startsWith(name, "ttyAMA") || startsWith(name, "rfcomm") ||
                startsWith(name, "ttyTHS") || startsWith(name, "ttyPS") ||
                startsWith(name, "ttyXRUSB") ||
                (startsWith(name, "ttyS") && isRealTtyS(name));
            if (!candidate)
                continue;

            std::error_code sec;
            if (fs::is_character_file(entry.path(), sec))
                ports.push_back(entry.path().string());
        }

        // USB adapters first: that's where an ESP32 usually is.
        auto rank = [](const std::string& p) {
            if (p.find("ttyUSB") != std::string::npos || p.find("ttyACM") != std::string::npos) return 0;
            if (p.find("ttyAMA") != std::string::npos || p.find("ttyS") != std::string::npos) return 1;
            return 2;
        };
        std::sort(ports.begin(), ports.end(), [&](const std::string& a, const std::string& b) {
            return rank(a) != rank(b) ? rank(a) < rank(b) : a < b;
        });
        ports.erase(std::unique(ports.begin(), ports.end()), ports.end());

        return ports;
    }
}
