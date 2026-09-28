#include "WinIPort.h"
#include <algorithm>
#include <thread>
#include <chrono>

namespace {
std::string normalize(const std::string& name) {
    if (name.rfind(R"(\\.\)", 0) == 0)
        return name;
    if (name.size() >= 4 && (name[0] == 'C' || name[0] == 'c') &&
        (name[1] == 'O' || name[1] == 'o') && (name[2] == 'M' || name[2] == 'm')) {
        return R"(\\.\)" + name;
    }
    return name;
}
}

bool WinIPort::open() {
    close();

    handle = ::CreateFileA(normalize(device).c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                            nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!::GetCommState(handle, &dcb)) { close(); return false; }

    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    if (!::SetCommState(handle, &dcb)) { close(); return false; }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    if (!::SetCommTimeouts(handle, &timeouts)) { close(); return false; }

    ::PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
}

void WinIPort::close() {
    if (handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

bool WinIPort::isOpen() const {
    return handle != INVALID_HANDLE_VALUE;
}

void WinIPort::flushBuss() {
    if (isOpen())
        ::FlushFileBuffers(handle);
}

std::vector<std::string> WinIPort::find() {
    std::vector<std::string> ports;
    for (int i = 1; i <= 64; ++i) {
        std::string name = "COM" + std::to_string(i);
        char target[512]{};
        if (::QueryDosDeviceA(name.c_str(), target, sizeof(target)) != 0)
            ports.push_back(name);
    }
    return ports;
}

void WinIPort::wait(uint32_t time_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
}

std::size_t WinIPort::rawwrite(std::span<const uint8_t> data, std::size_t size) {
    if (!isOpen() || data.empty() || size == 0) return 0;
    size = std::min(size, data.size());
    DWORD written = 0;
    if (!::WriteFile(handle, data.data(), static_cast<DWORD>(size), &written, nullptr))
        return 0;
    return written;
}

std::size_t WinIPort::rawread(std::span<uint8_t> buffer, std::size_t size) {
    if (!isOpen() || buffer.empty() || size == 0) return 0;
    size = std::min(size, buffer.size());

    // Non-blocking-ish poll: check bytes actually queued first, so we don't
    // block forever inside ReadFile (DBOXprotocol drives its own wait/retry loop).
    COMSTAT stat{};
    DWORD errors = 0;
    if (!::ClearCommError(handle, &errors, &stat))
        return 0;
    if (stat.cbInQue == 0)
        return 0;

    size = std::min<std::size_t>(size, stat.cbInQue);

    DWORD got = 0;
    if (!::ReadFile(handle, buffer.data(), static_cast<DWORD>(size), &got, nullptr))
        return 0;
    return got;
}
