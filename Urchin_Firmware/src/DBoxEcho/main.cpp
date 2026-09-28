// New file: minimal ESP32 firmware to verify DBOX protocol communication
// over a real UART link. Not part of Gabe's Clam/D-BOX code and not part
// of the normal Urchin firmware app -- built only under env:DBoxEcho for
// local hardware testing.
//
// Behavior: waits for a DBOX packet from the host, then sends the exact
// same packet back (echo), so the host can confirm real round-trip
// communication (framing + CRC) over the wire.

#include "Esp32UartPort.h"
#include <DBOXprotocol.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char* TAG = "DBoxEcho";

extern "C" void app_main() {
    // UART0 -- same physical USB-serial link used to flash/monitor the board.
    static Esp32UartPort port(UART_NUM_0, /*tx*/ 1, /*rx*/ 3, 115200);

    if (!port.open()) {
        ESP_LOGE(TAG, "Failed to open UART0");
        return;
    }

    DBOXProtocol::DBOXprotocol proto(&port, "/dboxdump.bin");

    while (true) {
        DBOXProtocol::DBOX incoming;

        if (proto.receive(&incoming)) {
            // Echo the exact same packet straight back.
            proto.send(&incoming);
        }
    }
}
