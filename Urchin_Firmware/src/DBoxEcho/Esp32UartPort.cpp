#include "Esp32UartPort.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

bool Esp32UartPort::open() {
    uart_config_t cfg = {};
    cfg.baud_rate = static_cast<int>(baud);
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    if (uart_driver_install(uartNum, 2048, 2048, 0, nullptr, 0) != ESP_OK)
        return false;
    if (uart_param_config(uartNum, &cfg) != ESP_OK)
        return false;
    if (uart_set_pin(uartNum, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
        return false;

    opened = true;
    return true;
}

void Esp32UartPort::close() {
    if (opened) {
        uart_driver_delete(uartNum);
        opened = false;
    }
}

bool Esp32UartPort::isOpen() const {
    return opened;
}

void Esp32UartPort::flushBuss() {
    if (opened)
        uart_flush(uartNum);
}

std::vector<std::string> Esp32UartPort::find() {
    return {};
}

void Esp32UartPort::wait(uint32_t time_ms) {
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}

std::size_t Esp32UartPort::rawwrite(std::span<const uint8_t> data, std::size_t size) {
    if (!opened)
        return 0;
    size = std::min(size, data.size());
    int written = uart_write_bytes(uartNum, reinterpret_cast<const char*>(data.data()), size);
    return written > 0 ? static_cast<std::size_t>(written) : 0;
}

std::size_t Esp32UartPort::rawread(std::span<uint8_t> buffer, std::size_t size) {
    if (!opened)
        return 0;
    size = std::min(size, buffer.size());
    int got = uart_read_bytes(uartNum, buffer.data(), size, 0);
    return got > 0 ? static_cast<std::size_t>(got) : 0;
}
