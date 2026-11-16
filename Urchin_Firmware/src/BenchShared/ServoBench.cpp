// See ServoBench.h.

#include "ServoBench.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// The header only declares `extern HerkulexClass Herkulex;` -- in the real
// app it's defined in Conversation/UnPacker.cpp, which the bench builds
// don't compile, so they supply the definition here.
HerkulexClass Herkulex;

namespace Bench {

static constexpr TickType_t kReplyTimeout = pdMS_TO_TICKS(30);

void Begin() {
    Herkulex.begin(kUart, kBaud, kRxPin, kTxPin);
    // Keep RX from floating (and reading noise) while the servo is
    // unpowered or unplugged.
    gpio_set_pull_mode(static_cast<gpio_num_t>(kRxPin), GPIO_PULLUP_ONLY);
}

static void SendPacket(uint8_t id, uint8_t cmd, const uint8_t* data, int len) {
    uint8_t pkt[16];
    uint8_t size = static_cast<uint8_t>(7 + len);
    uint8_t x = size ^ id ^ cmd;
    for (int i = 0; i < len; ++i) {
        x ^= data[i];
    }
    pkt[0] = 0xFF;
    pkt[1] = 0xFF;
    pkt[2] = size;
    pkt[3] = id;
    pkt[4] = cmd;
    pkt[5] = x & 0xFE;
    pkt[6] = (~x) & 0xFE;
    for (int i = 0; i < len; ++i) {
        pkt[7 + i] = data[i];
    }
    uart_write_bytes(kUart, pkt, size);
    uart_wait_tx_done(kUart, pdMS_TO_TICKS(20));
}

bool Ping(int id, ServoStatus* out, int* replyId) {
    uart_flush_input(kUart);  // drop noise / stale replies
    SendPacket(static_cast<uint8_t>(id), HSTAT, nullptr, 0);

    uint8_t r[9];
    if (uart_read_bytes(kUart, r, sizeof(r), kReplyTimeout) != static_cast<int>(sizeof(r))) {
        return false;
    }
    if (r[0] != 0xFF || r[1] != 0xFF || r[2] != 9 || r[4] != (HSTAT | 0x40)) {
        return false;
    }
    if (id != kBroadcastId && r[3] != id) {
        return false;
    }
    uint8_t x = r[2] ^ r[3] ^ r[4] ^ r[7] ^ r[8];
    if (r[5] != (x & 0xFE) || r[6] != ((~x) & 0xFE)) {
        return false;
    }
    out->error = r[7];
    out->detail = r[8];
    if (replyId) {
        *replyId = r[3];
    }
    return true;
}

void ClearErrors(int id) {
    const uint8_t data[] = {0x30, 0x02, 0x00, 0x00};
    SendPacket(static_cast<uint8_t>(id), HRAMWRITE, data, sizeof(data));
}

void Arm(int id) {
    ClearErrors(id);
    Herkulex.torqueON(id);
}

int FindServo(void (*progress)(int id)) {
    ServoStatus s;
    int replyId = -1;
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (Ping(kBroadcastId, &s, &replyId) && replyId < kBroadcastId) {
            return replyId;
        }
    }
    for (int id = 0; id < kBroadcastId; ++id) {
        if (progress) {
            progress(id);
        }
        if (Ping(id, &s, nullptr)) {
            return id;
        }
    }
    return -1;
}

void SpinAt(int id, int speed) {
    static constexpr int kSpinCommandMs = 300;
    int magnitude = (speed < 0) ? ((-speed) | 0x4000) : speed;
    int lsb = magnitude & 0x00FF;
    int msb = (magnitude & 0xFF00) >> 8;
    JogLedColor led = speed < 0 ? LED_RED : LED_BLUE;
    Herkulex.sendSJog(id, speed, kSpinCommandMs, led, kModel, /*Mode=*/true, lsb, msb);
}

void Stop(int id) {
    Herkulex.motor_stop(id);
}

bool MoveToAngle(int id, float degrees, int timeMs) {
    int position = static_cast<int>(degrees / kDegPerUnit) + kCenterPos;
    if (position < 0 || position > kMaxPos) {
        return false;
    }
    Herkulex.moveOne(id, position, timeMs, LED_GREEN, kModel);
    return true;
}

}  // namespace Bench
