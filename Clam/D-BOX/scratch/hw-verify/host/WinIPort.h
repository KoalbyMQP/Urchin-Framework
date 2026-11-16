// New file (not part of Gabe's Clam/D-BOX code): a working Windows COM-port
// backend for DBOX's IPort interface. The real one shipped in
// Transport-Interface/UART/Windows is an empty stub, so this fills that gap
// locally for a hardware verification test. Local-only, never committed.
#pragma once

#include <Transport-Interface/IPort.h>
#include <windows.h>
#include <string>
#include <vector>

class WinIPort : public DBOXProtocol::IPort {
public:
    WinIPort(std::string device, uint32_t baud) : device(std::move(device)), baud(baud) {}
    ~WinIPort() override { close(); }

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
    std::string device;
    uint32_t baud;
    HANDLE handle = INVALID_HANDLE_VALUE;
};
