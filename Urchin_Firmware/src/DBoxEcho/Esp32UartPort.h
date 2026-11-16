// New file (not part of Gabe's Clam/D-BOX code): a minimal ESP-IDF UART
// backend implementing D-BOX's IPort interface, written for a local
// hardware verification test. Local-only, not intended to be merged
// upstream into Clam/D-BOX.
#pragma once

#include <Transport-Interface/IPort.h>
#include <driver/uart.h>
#include <vector>
#include <string>

class Esp32UartPort : public DBOXProtocol::IPort {
public:
    Esp32UartPort(uart_port_t uartNum, int txPin, int rxPin, uint32_t baud)
        : uartNum(uartNum), txPin(txPin), rxPin(rxPin), baud(baud) {}

    bool open() override;
    void close() override;
    bool isOpen() const override;
    void flushBuss() override;
    std::vector<std::string> find() override;
    void wait(uint32_t time_ms) override;

protected:
    std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size) override;
    std::size_t rawread(std::span<uint8_t> buffer, std::size_t size) override;

private:
    uart_port_t uartNum;
    int txPin;
    int rxPin;
    uint32_t baud;
    bool opened = false;
};
