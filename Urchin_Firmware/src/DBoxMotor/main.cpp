// New file: ESP32 firmware for the end-to-end bench test
//   laptop/Pi --D-BOX over USB (UART0)--> ESP32 --Herkulex (UART1)--> servo
// Not part of the normal Urchin firmware app -- built only under
// env:DBoxMotor. Combines the two already-verified halves:
//   - D-BOX transport: the same vendored, unmodified D-BOX protocol and
//     Esp32UartPort that env:DBoxEcho proved against the host;
//   - servo control: the same src/BenchShared helpers env:HerkulexTest
//     proved against the servo.
// The host side is Crab_API/src/bench_test.py (pure-Python D-BOX in
// Crab_API/src/dbox.py).
//
// Wire protocol (all payloads are ASCII text):
//   handshake   host -> VPID 0, stream 'S', "Urchin"
//               ESP  -> VPID 0, stream 'S', "Urchin"
//               (mirrors Clam's UartDiscovery identity exchange)
//   commands    host -> stream 'N'; ESP answers each with exactly one
//               stream 'E' packet on the same VPID, "OK ..." or "ERR ...":
//     Info                -> OK DBoxMotor v1 servo=<id>|broadcast
//     Find                -> OK FOUND <id>  |  OK BLIND      (takes ~10s)
//     Status              -> OK STAT <id> <error> <detail>  |  ERR NOREPLY
//     Spin <speed>        -> OK SPIN <speed>      (-1023..1023, keep-alive)
//     Stop                -> OK STOP
//     Move <deg> <ms>     -> OK MOVE <deg>  |  ERR RANGE
//   events      ESP  -> stream 'D' (unsolicited), e.g. "AUTOSTOP"
//
// Spin is dead-man controlled: the host re-sends it every ~100ms while
// the user wants motion; if none arrives for kSpinDeadmanMs the servo is
// stopped. Closing the laptop program or pulling USB therefore always
// stops the motor.
//
// NO printf anywhere in this build: UART0 carries the D-BOX frames, and
// any console text would land in the middle of them. That includes the
// driver's own debug prints, which BenchShared/ComsStub.cpp mutes.

#include "../BenchShared/ServoBench.h"
#include "../DBoxEcho/Esp32UartPort.h"
#include <DBOXprotocol.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern bool g_driverLogsEnabled;

static constexpr const char* kIdentity = "Urchin";
static constexpr unsigned long kSpinRefreshMs = 150;
static constexpr unsigned long kSpinDeadmanMs = 500;
static constexpr int kMaxSpeed = 1023;

struct Command {
    uint8_t vpid;
    uint8_t stream;
    char text[64];
};

static QueueHandle_t g_commands;
static DBOXProtocol::DBOXprotocol* g_proto;

// The only task that reads UART0. DBOXprotocol::receive blocks until a
// whole, CRC-checked frame arrives, so it runs on its own and hands
// commands to the main loop, which owns everything else (including all
// sends).
static void ReceiveTask(void*) {
    while (true) {
        DBOXProtocol::DBOX incoming;
        if (!g_proto->receive(&incoming)) {
            continue;
        }
        Command cmd{};
        cmd.vpid = incoming.VPID;
        cmd.stream = incoming.Stream;
        size_t n = std::min(incoming.Payload.size(), sizeof(cmd.text) - 1);
        std::memcpy(cmd.text, incoming.Payload.data(), n);
        xQueueSend(g_commands, &cmd, portMAX_DELAY);
    }
}

static void Send(uint8_t vpid, char stream, const char* fmt, ...) {
    char buf[96];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    DBOXProtocol::DBOX packet(vpid, static_cast<uint8_t>(stream), buf);
    g_proto->send(&packet);
}

static bool StartsWith(const char* text, const char* word) {
    size_t n = std::strlen(word);
    return std::strncmp(text, word, n) == 0 && (text[n] == '\0' || text[n] == ' ');
}

extern "C" void app_main() {
    g_driverLogsEnabled = false;

    // UART0 -- same physical USB-serial link used to flash the board.
    static Esp32UartPort port(UART_NUM_0, /*tx*/ 1, /*rx*/ 3, 115200);
    if (!port.open()) {
        return;
    }
    static DBOXProtocol::DBOXprotocol proto(&port, "/dboxdump.bin");
    g_proto = &proto;

    Bench::Begin();
    vTaskDelay(pdMS_TO_TICKS(200));

    // Quick look for the servo (broadcast STAT only, well under a second)
    // so Status/Spin work straight away; the full ~10s scan only runs when
    // the host asks for it with Find.
    int servoId = Bench::kBroadcastId;
    {
        Bench::ServoStatus s;
        int replyId = -1;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (Bench::Ping(Bench::kBroadcastId, &s, &replyId) && replyId < Bench::kBroadcastId) {
                servoId = replyId;
                break;
            }
        }
    }

    g_commands = xQueueCreate(8, sizeof(Command));
    xTaskCreate(ReceiveTask, "DBoxRx", 8192, nullptr, 5, nullptr);

    int spinSpeed = 0;  // 0 = not spinning
    unsigned long lastSpinMsgMs = 0;
    unsigned long lastSpinCmdMs = 0;

    while (true) {
        Command cmd;
        if (xQueueReceive(g_commands, &cmd, pdMS_TO_TICKS(20)) == pdTRUE) {
            unsigned long now = millis();
            const char* t = cmd.text;
            uint8_t v = cmd.vpid;

            if (cmd.stream == 'S') {
                if (std::strcmp(t, kIdentity) == 0) {
                    Send(0, 'S', "%s", kIdentity);
                }
            } else if (cmd.stream != 'N') {
                // Not a command stream; ignore.
            } else if (StartsWith(t, "Info")) {
                if (servoId == Bench::kBroadcastId) {
                    Send(v, 'E', "OK DBoxMotor v1 servo=broadcast");
                } else {
                    Send(v, 'E', "OK DBoxMotor v1 servo=%d", servoId);
                }
            } else if (StartsWith(t, "Find")) {
                if (spinSpeed != 0) {
                    Bench::Stop(servoId);
                    spinSpeed = 0;
                }
                int id = Bench::FindServo(nullptr);
                if (id >= 0) {
                    servoId = id;
                    Bench::Arm(servoId);
                    Send(v, 'E', "OK FOUND %d", servoId);
                } else {
                    servoId = Bench::kBroadcastId;
                    Bench::Arm(servoId);
                    Send(v, 'E', "OK BLIND");
                }
            } else if (StartsWith(t, "Status")) {
                Bench::ServoStatus s;
                int replyId = servoId;
                if (Bench::Ping(servoId, &s, &replyId)) {
                    Send(v, 'E', "OK STAT %d %u %u", replyId, s.error, s.detail);
                } else {
                    Send(v, 'E', "ERR NOREPLY");
                }
            } else if (StartsWith(t, "Spin")) {
                int speed = std::atoi(t + 4);
                if (speed > kMaxSpeed) speed = kMaxSpeed;
                if (speed < -kMaxSpeed) speed = -kMaxSpeed;
                if (speed == 0) {
                    if (spinSpeed != 0) {
                        Bench::Stop(servoId);
                    }
                    spinSpeed = 0;
                } else if (speed != spinSpeed) {
                    // New spin (or new speed/direction): re-arm first so it
                    // works even if the servo was just powered on.
                    Bench::Arm(servoId);
                    Bench::SpinAt(servoId, speed);
                    spinSpeed = speed;
                    lastSpinCmdMs = now;
                } else if (now - lastSpinCmdMs > kSpinRefreshMs) {
                    Bench::SpinAt(servoId, speed);
                    lastSpinCmdMs = now;
                }
                lastSpinMsgMs = now;
                Send(v, 'E', "OK SPIN %d", speed);
            } else if (StartsWith(t, "Stop")) {
                Bench::Stop(servoId);
                spinSpeed = 0;
                Send(v, 'E', "OK STOP");
            } else if (StartsWith(t, "Move")) {
                char* end = nullptr;
                float degrees = std::strtof(t + 4, &end);
                int timeMs = (end && *end) ? std::atoi(end) : 1000;
                // HerkulexClass::moveOne silently ignores play times
                // outside 0-2856ms, so clamp rather than reply OK to a
                // move that never happens.
                if (timeMs < 0) timeMs = 0;
                if (timeMs > 2856) timeMs = 2856;
                if (end == t + 4) {
                    Send(v, 'E', "ERR USAGE Move <degrees> <ms>");
                } else {
                    if (spinSpeed != 0) {
                        Bench::Stop(servoId);
                        spinSpeed = 0;
                    }
                    Bench::Arm(servoId);
                    if (Bench::MoveToAngle(servoId, degrees, timeMs)) {
                        Send(v, 'E', "OK MOVE %.1f", static_cast<double>(degrees));
                    } else {
                        Send(v, 'E', "ERR RANGE");
                    }
                }
            } else {
                Send(v, 'E', "ERR UNKNOWN %s", t);
            }
        }

        // Dead-man: no Spin keep-alive recently -> stop.
        if (spinSpeed != 0 && millis() - lastSpinMsgMs > kSpinDeadmanMs) {
            Bench::Stop(servoId);
            spinSpeed = 0;
            Send(0, 'D', "AUTOSTOP");
        }
    }
}
