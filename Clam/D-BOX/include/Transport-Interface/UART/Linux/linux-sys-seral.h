//
// Created by gabri on 6/18/2026.
// Updated 2026-11-03 to the current IPort / UARTSerialPort<UartDiscovery>
// interface (the original targeted the old sys-seral.h API and no longer
// compiled).
//

#ifndef D_BOX_LINUX_SYS_SERAL_H
#define D_BOX_LINUX_SYS_SERAL_H

#include <Transport-Interface/UART/UARTSeralPort.h>
#include <Discovery/UartDiscovery.h>

#include <termios.h>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace DBOXProtocol {

    /**
     * @class   LinuxSerialPort
     * @brief   IPort backend for real serial devices on Linux (Raspberry Pi,
     *          WSL with a USB device attached, Linux PCs).
     *
     * Reads wait at most the configured timeout (DconfigUart::timeout, or the
     * timeoutMs passed to openraw) and then return 0, so DBOXprotocol's
     * receive() timeout and UartDiscovery can give up on silent ports.
     *
     * Opening a port leaves DTR and RTS deasserted: on ESP32 dev boards those
     * lines drive EN/BOOT, and toggling them resets the chip.
     */
    class LinuxSerialPort final : public UARTSerialPort<UartDiscovery> {
    public:
        LinuxSerialPort() : UARTSerialPort() {}
        explicit LinuxSerialPort(UartDiscovery* Discovery) : UARTSerialPort(Discovery) {}
        ~LinuxSerialPort() override;

        LinuxSerialPort(const LinuxSerialPort&) = delete;
        LinuxSerialPort& operator=(const LinuxSerialPort&) = delete;

        /** Opens the port named in the bound Discovery's config. */
        bool open() override;

        /** Opens `device` (e.g. "/dev/ttyUSB0") at `baudRate`, 8N1, raw. */
        bool openraw(const char* device, uint32_t baudRate, uint32_t timeoutMs) override;

        void close() override;
        bool isOpen() const override;

        /** Discards anything waiting in the OS input and output buffers. */
        void flushBuss() override;

        /** Lists serial devices that actually exist (see .cpp for rules). */
        std::vector<std::string> find() override;

        void wait(uint32_t time_ms) override;

    protected:
        std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size) override;
        std::size_t rawread(std::span<uint8_t> buffer, std::size_t size) override;

    private:
        int m_fd = -1;
        uint32_t m_timeoutMs = 100;

        static bool configurePort(int fd, uint32_t baudRate);
        static speed_t toBaud(uint32_t baudRate);
    };
}
#endif //D_BOX_LINUX_SYS_SERAL_H
