// Host simulation of the DBoxMotor board: the real firmware sources run on
// Linux, with
//   UART0 -> a pty (the bench test opens its slave end like a /dev/ttyUSB0)
//   UART1 -> simulated Herkulex servos on one daisy-chained bus.
// SIM_SERVOS="219:0601,253:0201@57600" picks the servos (optional
// @baud: a servo set to another speed only hears/answers at that speed); SIM_LOG=<file> records
// what each servo was told to do.
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstdarg>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <mutex>
#include <pty.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

// --- FreeRTOS ---------------------------------------------------------------
static const auto kStart = std::chrono::steady_clock::now();
TickType_t xTaskGetTickCount() {
    return (TickType_t)std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - kStart).count();
}
void vTaskDelay(TickType_t t) { std::this_thread::sleep_for(std::chrono::milliseconds(t)); }
BaseType_t xTaskCreate(TaskFunction_t f, const char*, uint32_t, void* arg, UBaseType_t, TaskHandle_t*) {
    std::thread([f, arg] { f(arg); }).detach();
    return pdPASS;
}
struct SimQueue {
    std::mutex m;
    std::condition_variable cv;
    std::deque<std::vector<uint8_t>> items;
    size_t len, size;
};
QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t size) { return new SimQueue{{}, {}, {}, len, size}; }
BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t) {
    std::lock_guard<std::mutex> l(q->m);
    const uint8_t* p = (const uint8_t*)item;
    q->items.emplace_back(p, p + q->size);
    q->cv.notify_one();
    return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t wait) {
    std::unique_lock<std::mutex> l(q->m);
    if (!q->cv.wait_for(l, std::chrono::milliseconds(wait == portMAX_DELAY ? 1000000000u : wait),
                        [q] { return !q->items.empty(); }))
        return pdFALSE;
    memcpy(item, q->items.front().data(), q->size);
    q->items.pop_front();
    return pdTRUE;
}

// --- Simulated Herkulex bus ---------------------------------------------------
struct SimServo {
    int id, eepId;
    uint8_t model[2];
    const char* name;
    bool torque = false;
    uint8_t err = 0, det = 0;
    unsigned long bootUntil = 0;
    int baud = 115200, eepBaud = 115200;
};
static int g_busBaud = 115200;
static int BaudFromCode(uint8_t code) {
    switch (code) {
        case 0x02: return 666666; case 0x03: return 500000; case 0x04: return 400000;
        case 0x07: return 250000; case 0x09: return 200000; case 0x10: return 115200;
        case 0x22: return 57600; default: return 0;
    }
}
static std::vector<SimServo> g_servos;
static std::mutex g_busMutex;
static std::condition_variable g_busCv;
static std::deque<uint8_t> g_busRx;   // servo -> ESP32
static std::vector<uint8_t> g_busTx;  // ESP32 -> servos, not yet parsed
static FILE* g_log = nullptr;

static void Log(const char* fmt, ...) {
    if (!g_log) return;
    va_list a;
    va_start(a, fmt);
    fprintf(g_log, "%7u ", (unsigned)xTaskGetTickCount());
    vfprintf(g_log, fmt, a);
    fputc('\n', g_log);
    fflush(g_log);
    va_end(a);
}

static std::vector<uint8_t> Packet(uint8_t id, uint8_t cmd, std::vector<uint8_t> data) {
    uint8_t size = 7 + data.size();
    uint8_t x = size ^ id ^ cmd;
    for (uint8_t b : data) x ^= b;
    std::vector<uint8_t> p{0xFF, 0xFF, size, id, cmd, (uint8_t)(x & 0xFE), (uint8_t)(~x & 0xFE)};
    p.insert(p.end(), data.begin(), data.end());
    return p;
}

static bool Alive(const SimServo& s) { return xTaskGetTickCount() >= s.bootUntil; }

static void HandleBusPacket(const std::vector<uint8_t>& p) {
    uint8_t size = p[2], id = p[3], cmd = p[4];
    uint8_t x = size ^ id ^ cmd;
    for (size_t i = 7; i < size; ++i) x ^= p[i];
    if (p[5] != (x & 0xFE) || p[6] != (uint8_t)(~x & 0xFE)) {
        Log("BUS bad checksum (cmd 0x%02X id %d) -- servo would ignore it", cmd, id);
        return;
    }
    const uint8_t* d = p.data() + 7;
    int n = size - 7;
    std::vector<std::vector<uint8_t>> replies;
    for (SimServo& s : g_servos) {
        if (!Alive(s) || s.baud != g_busBaud) continue;  // wrong speed: hears noise
        bool mine = (id == s.id || id == 0xFE);
        switch (cmd) {
            case 0x07:  // STAT
                if (mine) replies.push_back(Packet(s.id, 0x47, {s.err, (uint8_t)(s.det | (s.torque ? 0x40 : 0))}));
                break;
            case 0x02:  // EEP_READ
                if (id == s.id) {
                    std::vector<uint8_t> r{d[0], d[1]};
                    for (int a = d[0]; a < d[0] + d[1]; ++a)
                        r.push_back(a < 2 ? s.model[a] : a == 6 ? s.eepId : 0);
                    r.push_back(s.err);
                    r.push_back(s.det);
                    replies.push_back(Packet(s.id, 0x42, r));
                }
                break;
            case 0x01:  // EEP_WRITE
                if (mine && d[0] == 6) {
                    s.eepId = d[2];
                    Log("SERVO %d (%s) EEPROM ID <- %d (after reboot)", s.id, s.name, s.eepId);
                }
                if (mine && d[0] == 4 && BaudFromCode(d[2])) {
                    s.eepBaud = BaudFromCode(d[2]);
                    Log("SERVO %d (%s) EEPROM baud <- %d (after reboot)", s.id, s.name, s.eepBaud);
                }
                break;
            case 0x03:  // RAM_WRITE
                if (!mine) break;
                if (d[0] == 0x30) { s.err = s.det = 0; }
                else if (d[0] == 0x34) {
                    bool on = d[2] == 0x60;
                    if (on != s.torque) Log("SERVO %d (%s) torque %s", s.id, s.name, on ? "ON" : "OFF");
                    s.torque = on;
                }
                break;
            case 0x09:  // REBOOT
                if (mine) {
                    Log("SERVO %d (%s) reboot -> ID %d", s.id, s.name, s.eepId);
                    s.id = s.eepId;
                    s.baud = s.eepBaud;
                    s.torque = false;
                    s.bootUntil = xTaskGetTickCount() + 300;
                }
                break;
            case 0x06: {  // S_JOG: playtime, then (LSB MSB SET ID) per servo
                for (int i = 1; i + 3 < n; i += 4) {
                    if (d[i + 3] != s.id && d[i + 3] != 0xFE) continue;
                    int raw = d[i] | (d[i + 1] << 8);
                    bool speedMode = d[i + 2] & 0x02;
                    if (speedMode) {
                        int speed = (raw & 0x4000) ? -(raw & 0x3FFF) : raw;
                        Log("SERVO %d (%s) %s", s.id, s.name,
                            speed == 0 ? "STOP" : (std::string("SPIN ") + std::to_string(speed) +
                                                   (s.torque ? "" : " (torque OFF: no motion)")).c_str());
                    } else {
                        Log("SERVO %d (%s) MOVE to position %d%s", s.id, s.name, raw,
                            s.torque ? "" : " (torque OFF: no motion)");
                    }
                }
                break;
            }
        }
    }
    if (replies.empty()) return;
    std::vector<uint8_t> out = replies[0];
    if (replies.size() > 1) {  // several servos talking at once: garbage
        for (size_t r = 1; r < replies.size(); ++r)
            for (size_t i = 0; i < out.size() && i < replies[r].size(); ++i) out[i] &= replies[r][i];
        Log("BUS %zu servos answered at once -> collision", replies.size());
    }
    std::lock_guard<std::mutex> l(g_busMutex);
    g_busRx.insert(g_busRx.end(), out.begin(), out.end());
    g_busCv.notify_all();
}

static void BusWrite(const uint8_t* b, size_t n) {
    g_busTx.insert(g_busTx.end(), b, b + n);
    while (true) {
        size_t i = 0;
        while (i + 1 < g_busTx.size() && !(g_busTx[i] == 0xFF && g_busTx[i + 1] == 0xFF)) ++i;
        g_busTx.erase(g_busTx.begin(), g_busTx.begin() + i);
        if (g_busTx.size() < 3 || g_busTx.size() < g_busTx[2]) return;
        if (g_busTx[2] < 7) { g_busTx.erase(g_busTx.begin()); continue; }
        std::vector<uint8_t> p(g_busTx.begin(), g_busTx.begin() + g_busTx[2]);
        g_busTx.erase(g_busTx.begin(), g_busTx.begin() + p[2]);
        HandleBusPacket(p);
    }
}

// --- UART driver ----------------------------------------------------------------
static int g_ptyMaster = -1;
esp_err_t uart_driver_install(uart_port_t, int, int, int, void*, int) { return ESP_OK; }
esp_err_t uart_driver_delete(uart_port_t) { return ESP_OK; }
esp_err_t uart_param_config(uart_port_t, const uart_config_t*) { return ESP_OK; }
esp_err_t uart_set_pin(uart_port_t, int, int, int, int) { return ESP_OK; }
esp_err_t uart_wait_tx_done(uart_port_t, TickType_t) { return ESP_OK; }
esp_err_t uart_set_baudrate(uart_port_t u, uint32_t baud) {
    if (u == UART_NUM_1) { g_busBaud = (int)baud; g_busTx.clear(); }
    return ESP_OK;
}
int uart_write_bytes(uart_port_t u, const void* src, size_t size) {
    if (u == UART_NUM_0) return (int)write(g_ptyMaster, src, size);
    BusWrite((const uint8_t*)src, size);
    return (int)size;
}
int uart_read_bytes(uart_port_t u, void* buf, uint32_t length, TickType_t wait) {
    if (u == UART_NUM_0) {
        ssize_t r = read(g_ptyMaster, buf, length);  // non-blocking
        if (r <= 0 && wait) vTaskDelay(1);
        return r < 0 ? 0 : (int)r;
    }
    std::unique_lock<std::mutex> l(g_busMutex);
    g_busCv.wait_for(l, std::chrono::milliseconds(wait), [&] { return g_busRx.size() >= length; });
    size_t n = std::min<size_t>(length, g_busRx.size());
    for (size_t i = 0; i < n; ++i) { ((uint8_t*)buf)[i] = g_busRx.front(); g_busRx.pop_front(); }
    return (int)n;
}
esp_err_t uart_flush_input(uart_port_t u) {
    if (u == UART_NUM_1) { std::lock_guard<std::mutex> l(g_busMutex); g_busRx.clear(); }
    return ESP_OK;
}
esp_err_t uart_flush(uart_port_t u) { return uart_flush_input(u); }

// --- main -------------------------------------------------------------------------
extern "C" void app_main();

int main() {
    const char* spec = getenv("SIM_SERVOS");
    std::string list = spec ? spec : "219:0601";
    for (size_t pos = 0; pos < list.size();) {
        size_t comma = list.find(',', pos);
        std::string item = list.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        pos = comma == std::string::npos ? list.size() : comma + 1;
        if (item.empty()) continue;
        SimServo s{};
        s.id = s.eepId = atoi(item.c_str());
        std::string m = item.substr(item.find(':') + 1);
        if (m.find('@') != std::string::npos) {
            s.baud = s.eepBaud = std::stoi(m.substr(m.find('@') + 1));
            m = m.substr(0, m.find('@'));
        }
        static std::deque<std::string> names;
        names.push_back("DRS-" + m);
        s.name = names.back().c_str();
        s.model[0] = (uint8_t)std::stoi(m.substr(0, 2), nullptr, 16);  // "06" -> 0x06
        s.model[1] = (uint8_t)std::stoi(m.substr(2, 2), nullptr, 16);  // "01" -> 0x01
        g_servos.push_back(s);
    }
    if (const char* lf = getenv("SIM_LOG")) g_log = fopen(lf, "w");

    int slave;
    if (openpty(&g_ptyMaster, &slave, nullptr, nullptr, nullptr) != 0) { perror("openpty"); return 1; }
    termios t;
    tcgetattr(slave, &t);
    cfmakeraw(&t);
    tcsetattr(slave, TCSANOW, &t);
    fcntl(g_ptyMaster, F_SETFL, O_NONBLOCK);
    printf("%s\n", ttyname(slave));
    fflush(stdout);
    for (auto& s : g_servos) Log("BUS servo %s at ID %d, %d baud", s.name, s.id, s.baud);
    app_main();
}
