//
// Created by gabri on 6/18/2026.
//

#include "WindowsSerialPort.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <windows.h>

namespace
{
    std::string normalizePortName(const char* device)
    {
        if (device == nullptr || *device == '\0')
            return {};

        std::string name(device);

        // Already in Win32 device-path form
        if (name.rfind(R"(\\.\)", 0) == 0)
            return name;

        // If it's COMx, convert COM10+ to \\.\COM10 form.
        // Using \\.\COMx for all COM ports is also fine.
        if (name.size() >= 4 &&
            (name[0] == 'C' || name[0] == 'c') &&
            (name[1] == 'O' || name[1] == 'o') &&
            (name[2] == 'M' || name[2] == 'm'))
        {
            return R"(\\.\)" + name;
        }

        // Otherwise leave as-is
        return name;
    }
}

WindowsSerialPort::~WindowsSerialPort()
{
    close();
}

bool WindowsSerialPort::configurePort(HANDLE handle, uint32_t baudRate)
{
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);

    if (!::GetCommState(handle, &dcb))
        return false;

    // 8N1
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    // Binary mode, no parity checking
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    // No hardware flow control
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_DISABLE;
    dcb.fOutX        = FALSE;
    dcb.fInX         = FALSE;

    if (!::SetCommState(handle, &dcb))
        return false;

    // Blocking read semantics similar to Linux:
    // wait until at least one byte is available, then return.
    //
    // With ReadIntervalTimeout/MAXDWORD and multipliers/constants 0,
    // ReadFile blocks until at least one byte is available.
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout         = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier  = 0;
    timeouts.ReadTotalTimeoutConstant    = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant   = 0;

    if (!::SetCommTimeouts(handle, &timeouts))
        return false;

    // Set internal driver queue sizes (best effort)
    ::SetupComm(handle, 4096, 4096);

    // Clear stale data on open/configure
    ::PurgeComm(handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    return true;
}

bool WindowsSerialPort::open(const char* device, uint32_t baudRate)
{
    close();

    const std::string portName = normalizePortName(device);
    if (portName.empty())
        return false;

    HANDLE handle = ::CreateFileA(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,                    // exclusive access
        nullptr,
        OPEN_EXISTING,
        0,                    // synchronous / blocking I/O
        nullptr);

    if (handle == INVALID_HANDLE_VALUE)
        return false;

    if (!configurePort(handle, baudRate))
    {
        ::CloseHandle(handle);
        return false;
    }

    m_handle = handle;
    return true;
}

void WindowsSerialPort::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool WindowsSerialPort::isOpen() const
{
    return m_handle != INVALID_HANDLE_VALUE;
}

std::size_t WindowsSerialPort::rawwrite(std::span<const uint8_t> data, std::size_t size)
{
    if (!isOpen() || data.empty() || size == 0)
        return 0;

    size = std::min(size, data.size());

    std::size_t totalWritten = 0;
    while (totalWritten < size)
    {
        const DWORD chunk =
            static_cast<DWORD>(std::min<std::size_t>(size - totalWritten, 0xFFFFFFFFu));

        DWORD written = 0;
        const BOOL ok = ::WriteFile(
            m_handle,
            data.data() + totalWritten,
            chunk,
            &written,
            nullptr);

        if (!ok)
            break;

        if (written == 0)
            break;

        totalWritten += static_cast<std::size_t>(written);
    }

    return totalWritten;
}

std::size_t WindowsSerialPort::rawread(std::span<uint8_t> buffer, std::size_t size)
{
    if (!isOpen() || buffer.empty() || size == 0)
        return 0;

    size = std::min(size, buffer.size());

    const DWORD chunk =
        static_cast<DWORD>(std::min<std::size_t>(size, 0xFFFFFFFFu));

    DWORD bytesRead = 0;
    const BOOL ok = ::ReadFile(
        m_handle,
        buffer.data(),
        chunk,
        &bytesRead,
        nullptr);

    if (!ok)
        return 0;

    return static_cast<std::size_t>(bytesRead);
}

void WindowsSerialPort::flushBus()
{
    if (!isOpen())
        return;

    // FlushFileBuffers blocks until buffered data is written
    ::FlushFileBuffers(m_handle);
}

void WindowsSerialPort::clearInputBuffer()
{
    if (!isOpen())
        return;

    ::PurgeComm(m_handle, PURGE_RXCLEAR);
}

void WindowsSerialPort::clearOutputBuffer()
{
    if (!isOpen())
        return;

    ::PurgeComm(m_handle, PURGE_TXCLEAR);
}

std::vector<std::string> WindowsSerialPort::find()
{
    std::vector<std::string> ports;

    // Probe COM1..COM256
    // QueryDosDeviceA returns non-zero if the DOS device exists.
    for (int i = 1; i <= 256; ++i)
    {
        const std::string name = "COM" + std::to_string(i);

        char target[512]{};
        if (::QueryDosDeviceA(name.c_str(), target, static_cast<DWORD>(std::size(target))) != 0)
        {
            ports.push_back(name);
        }
    }

    return ports;
}