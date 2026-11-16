#ifdef URCHIN_DBOX

#include "DboxLink.h"
#include "Coms.h" // UART_NUM

#include <DBOXprotocol.h>
#include <Transport-Interface/IPort.h>
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <algorithm>
#include <cstring>

namespace {

// UART0 is already installed and configured by SetUpUart(), so this port only reads and writes it.
class UrchinUartPort : public DBOXProtocol::IPort {
public:
    bool open() override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    void flushBuss() override { uart_flush(UART_NUM); }
    std::vector<std::string> find() override { return {}; }
    void wait(uint32_t time_ms) override { vTaskDelay(pdMS_TO_TICKS(time_ms)); }

protected:
    std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size) override {
        size = std::min(size, data.size());
        int written = uart_write_bytes(UART_NUM, data.data(), size);
        return written > 0 ? static_cast<std::size_t>(written) : 0;
    }

    std::size_t rawread(std::span<uint8_t> buffer, std::size_t size) override {
        size = std::min(size, buffer.size());
        int got = uart_read_bytes(UART_NUM, buffer.data(), size, 0);
        return got > 0 ? static_cast<std::size_t>(got) : 0;
    }
};

UrchinUartPort *g_port = nullptr;
DBOXProtocol::DBOXprotocol *g_proto = nullptr;
SemaphoreHandle_t g_sendLock = nullptr; // one packet on the wire at a time

} // namespace

extern "C" int DboxLinkInit(void) {
    static UrchinUartPort port;
    static DBOXProtocol::DBOXprotocol proto(&port, "/dev/null");
    g_port = &port;
    g_proto = &proto;
    g_sendLock = xSemaphoreCreateMutex();
    return g_sendLock != nullptr;
}

extern "C" int DboxSend(uint8_t vpid, char stream, const uint8_t *data, size_t size) {
    if (g_proto == nullptr) {
        return -1;
    }
    DBOXProtocol::DBOX packet;
    packet.VPID = vpid;
    packet.Stream = static_cast<uint8_t>(stream);
    packet.Payload.assign(data, data + size);

    xSemaphoreTake(g_sendLock, portMAX_DELAY);
    bool ok = g_proto->send(&packet);
    xSemaphoreGive(g_sendLock);
    return ok ? 0 : -2;
}

extern "C" int DboxReceive(uint8_t *vpid, uint8_t *stream, uint8_t *data, size_t capacity) {
    if (g_proto == nullptr) {
        return 0;
    }
    DBOXProtocol::DBOX packet;
    if (!g_proto->receive(&packet)) {
        return 0;
    }
    *vpid = packet.VPID;
    *stream = packet.Stream;
    size_t n = std::min(packet.Payload.size(), capacity);
    std::memcpy(data, packet.Payload.data(), n);
    std::memset(data + n, 0, capacity - n);
    return 1;
}

#endif // URCHIN_DBOX
