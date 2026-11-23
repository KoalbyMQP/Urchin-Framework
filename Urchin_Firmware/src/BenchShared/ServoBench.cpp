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

static const ModelInfo kModels[] = {
    {MODEL_0101, "0101", 512, 0.325f, 1023},
    {MODEL_0201, "0201", 512, 0.325f, 1023},
    {MODEL_0601, "0601", 1024, 0.161f, 2047},
    {MODEL_0602, "0602", 16384, 0.011f, 32767},
};

const ModelInfo& InfoFor(HerkulexModel model) {
    for (const ModelInfo& m : kModels) {
        if (m.model == model) {
            return m;
        }
    }
    return kModels[2];  // MODEL_0601, the bench default
}

bool ParseModel(const char* text, HerkulexModel* out) {
    if (text[0] == 'D' && text[1] == 'R' && text[2] == 'S' && text[3] == '-') {
        text += 4;
    }
    for (const ModelInfo& m : kModels) {
        int i = 0;
        while (m.name[i] && text[i] == m.name[i]) {
            ++i;
        }
        if (m.name[i] == '\0' && (text[i] == '\0' || text[i] == ' ')) {
            *out = m.model;
            return true;
        }
    }
    return false;
}

// Bus 2's driver instance (bus 1 is the global Herkulex above).
static HerkulexClass Herkulex2;
static int g_bus = 1;

static uart_port_t CurUart() {
    return g_bus == 2 ? kUart2 : kUart;
}

static HerkulexClass& Cur() {
    return g_bus == 2 ? Herkulex2 : Herkulex;
}

void SelectBus(int bus) {
    g_bus = (bus == 2) ? 2 : 1;
}

int SelectedBus() {
    return g_bus;
}

void BeginBus2() {
    Herkulex2.begin(kUart2, kBaud, kRxPin2, kTxPin2);
    gpio_set_pull_mode(static_cast<gpio_num_t>(kRxPin2), GPIO_PULLUP_ONLY);
}

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
    uart_write_bytes(CurUart(), pkt, size);
    uart_wait_tx_done(CurUart(), pdMS_TO_TICKS(20));
}

bool Ping(int id, ServoStatus* out, int* replyId) {
    uart_flush_input(CurUart());  // drop noise / stale replies
    SendPacket(static_cast<uint8_t>(id), HSTAT, nullptr, 0);

    uint8_t r[9];
    if (uart_read_bytes(CurUart(), r, sizeof(r), kReplyTimeout) != static_cast<int>(sizeof(r))) {
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

bool ReadEep(int id, uint8_t addr, uint8_t len, uint8_t* out) {
    if (len > 8) {
        return false;
    }
    uart_flush_input(CurUart());
    const uint8_t data[] = {addr, len};
    SendPacket(static_cast<uint8_t>(id), HEEPREAD, data, sizeof(data));

    // Reply: FF FF size id 0x42 ck1 ck2 addr len data[len] error detail
    uint8_t r[11 + 8];
    int want = 11 + len;
    if (uart_read_bytes(CurUart(), r, want, kReplyTimeout) != want) {
        return false;
    }
    if (r[0] != 0xFF || r[1] != 0xFF || r[2] != want || r[3] != id ||
        r[4] != (HEEPREAD | 0x40) || r[7] != addr || r[8] != len) {
        return false;
    }
    uint8_t x = r[2] ^ r[3] ^ r[4];
    for (int i = 7; i < want; ++i) {
        x ^= r[i];
    }
    if (r[5] != (x & 0xFE) || r[6] != ((~x) & 0xFE)) {
        return false;
    }
    for (int i = 0; i < len; ++i) {
        out[i] = r[9 + i];
    }
    return true;
}

bool ReadModel(int id, HerkulexModel* out) {
    uint8_t m[2];
    if (!ReadEep(id, 0x00, 2, m)) {
        return false;
    }
    switch (m[0]) {
        case 0x01: *out = MODEL_0101; return true;
        case 0x02: *out = MODEL_0201; return true;
        case 0x04:
        case 0x06: *out = (m[1] == 0x02) ? MODEL_0602 : MODEL_0601; return true;
        default: return false;
    }
}

int FindAll(int* ids, int maxIds) {
    ServoStatus s;
    int count = 0;
    for (int id = 0; id <= kMaxServoId && count < maxIds; ++id) {
        if (Ping(id, &s, nullptr)) {
            ids[count++] = id;
        }
    }
    return count;
}

bool ChangeId(int oldId, int newId) {
    // EEPROM address 6 holds the ID on every DRS family; it is copied into
    // RAM (and so takes effect) at boot, hence the reboot.
    const uint8_t data[] = {0x06, 0x01, static_cast<uint8_t>(newId)};
    SendPacket(static_cast<uint8_t>(oldId), HEEPWRITE, data, sizeof(data));
    vTaskDelay(pdMS_TO_TICKS(50));
    SendPacket(static_cast<uint8_t>(oldId), HREBOOT, nullptr, 0);
    ServoStatus s;
    for (int i = 0; i < 20; ++i) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (Ping(newId, &s, nullptr)) {
            return true;
        }
    }
    return false;
}

const BaudInfo kBauds[7] = {
    {115200, 0x10}, {57600, 0x22}, {200000, 0x09}, {250000, 0x07},
    {400000, 0x04}, {500000, 0x03}, {666666, 0x02},
};

static void SetBusBaud(int baud) {
    uart_set_baudrate(CurUart(), baud);
    uart_flush_input(CurUart());
}

int ScanBauds(int* id) {
    ServoStatus s;
    int found = -1;
    for (const BaudInfo& b : kBauds) {
        SetBusBaud(b.baud);
        vTaskDelay(pdMS_TO_TICKS(5));
        int replyId = -1;
        for (int attempt = 0; attempt < 3 && found < 0; ++attempt) {
            if (Ping(kBroadcastId, &s, &replyId) && replyId <= kMaxServoId) {
                found = b.baud;
                *id = replyId;
            }
        }
        if (found >= 0) {
            break;
        }
    }
    SetBusBaud(kBaud);
    return found;
}

bool ResetBaud(int id, int currentBaud) {
    uint8_t code = 0;
    for (const BaudInfo& b : kBauds) {
        if (b.baud == kBaud) {
            code = b.code;
        }
    }
    SetBusBaud(currentBaud);
    vTaskDelay(pdMS_TO_TICKS(5));
    // EEPROM address 4 = baud rate; it takes effect after a reboot.
    const uint8_t data[] = {0x04, 0x01, code};
    SendPacket(static_cast<uint8_t>(id), HEEPWRITE, data, sizeof(data));
    vTaskDelay(pdMS_TO_TICKS(50));
    SendPacket(static_cast<uint8_t>(id), HREBOOT, nullptr, 0);
    SetBusBaud(kBaud);
    ServoStatus s;
    for (int i = 0; i < 20; ++i) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (Ping(id, &s, nullptr)) {
            return true;
        }
    }
    return false;
}

int SniffStat(int id, uint8_t* out, int max, int* idleBytes) {
    uint8_t junk[64];
    uart_flush_input(CurUart());
    *idleBytes = uart_read_bytes(CurUart(), junk, sizeof(junk), pdMS_TO_TICKS(100));
    uart_flush_input(CurUart());
    SendPacket(static_cast<uint8_t>(id), HSTAT, nullptr, 0);
    int got = uart_read_bytes(CurUart(), out, max, pdMS_TO_TICKS(50));
    return got < 0 ? 0 : got;
}

bool PositionFor(float degrees, HerkulexModel model, int* position) {
    const ModelInfo& m = InfoFor(model);
    int p = static_cast<int>(degrees / m.degPerUnit) + m.centerPos;
    if (p < 0 || p > m.maxPos) {
        return false;
    }
    *position = p;
    return true;
}

// Herkulex play time is in units of 11.2ms, 0..255 (0..2856ms).
static uint8_t PlayTime(int timeMs) {
    int units = static_cast<int>(static_cast<float>(timeMs) / 11.2f);
    return static_cast<uint8_t>(units < 0 ? 0 : (units > 255 ? 255 : units));
}

// SET byte for "position control, green LED" (manual: bit1 = mode 0 for
// position, bits 2-4 = green/blue/red LED).
static constexpr uint8_t kSetPositionGreen = 0x04;

static void SendLongPacket(uint8_t cmd, const uint8_t* data, int len) {
    uint8_t pkt[7 + 5 * kMaxJogTargets];
    uint8_t size = static_cast<uint8_t>(7 + len);
    uint8_t x = size ^ kBroadcastId ^ cmd;
    for (int i = 0; i < len; ++i) {
        x ^= data[i];
    }
    pkt[0] = 0xFF;
    pkt[1] = 0xFF;
    pkt[2] = size;
    pkt[3] = kBroadcastId;
    pkt[4] = cmd;
    pkt[5] = x & 0xFE;
    pkt[6] = (~x) & 0xFE;
    for (int i = 0; i < len; ++i) {
        pkt[7 + i] = data[i];
    }
    uart_write_bytes(CurUart(), pkt, size);
    uart_wait_tx_done(CurUart(), pdMS_TO_TICKS(30));
}

bool SyncMove(const JogTarget* targets, int count, int timeMs) {
    if (count < 1 || count > kMaxJogTargets) {
        return false;
    }
    uint8_t data[1 + 4 * kMaxJogTargets];
    int len = 0;
    data[len++] = PlayTime(timeMs);
    for (int i = 0; i < count; ++i) {
        data[len++] = static_cast<uint8_t>(targets[i].position & 0xFF);
        data[len++] = static_cast<uint8_t>((targets[i].position >> 8) & 0x7F);
        data[len++] = kSetPositionGreen;
        data[len++] = static_cast<uint8_t>(targets[i].id);
    }
    SendLongPacket(HSIMULJOG, data, len);
    return true;
}

bool SyncMoveEach(const JogTarget* targets, int count) {
    if (count < 1 || count > kMaxJogTargets) {
        return false;
    }
    uint8_t data[5 * kMaxJogTargets];
    int len = 0;
    for (int i = 0; i < count; ++i) {
        data[len++] = static_cast<uint8_t>(targets[i].position & 0xFF);
        data[len++] = static_cast<uint8_t>((targets[i].position >> 8) & 0x7F);
        data[len++] = kSetPositionGreen;
        data[len++] = static_cast<uint8_t>(targets[i].id);
        data[len++] = PlayTime(targets[i].timeMs);
    }
    SendLongPacket(HINDEJOG, data, len);
    return true;
}

void ClearErrors(int id) {
    const uint8_t data[] = {0x30, 0x02, 0x00, 0x00};
    SendPacket(static_cast<uint8_t>(id), HRAMWRITE, data, sizeof(data));
}

void Arm(int id) {
    ClearErrors(id);
    Cur().torqueON(id);
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
    SpinAt(id, speed, kModel);
}

void SpinAt(int id, int speed, HerkulexModel model) {
    static constexpr int kSpinCommandMs = 300;
    int magnitude = (speed < 0) ? ((-speed) | 0x4000) : speed;
    int lsb = magnitude & 0x00FF;
    int msb = (magnitude & 0xFF00) >> 8;
    JogLedColor led = speed < 0 ? LED_RED : LED_BLUE;
    Cur().sendSJog(id, speed, kSpinCommandMs, led, model, /*Mode=*/true, lsb, msb);
}

void Stop(int id) {
    Cur().motor_stop(id);
}

bool MoveToAngle(int id, float degrees, int timeMs) {
    return MoveToAngle(id, degrees, timeMs, kModel);
}

bool MoveToAngle(int id, float degrees, int timeMs, HerkulexModel model) {
    const ModelInfo& m = InfoFor(model);
    int position = static_cast<int>(degrees / m.degPerUnit) + m.centerPos;
    if (position < 0 || position > m.maxPos) {
        return false;
    }
    Cur().moveOne(id, position, timeMs, LED_GREEN, model);
    return true;
}

}  // namespace Bench
