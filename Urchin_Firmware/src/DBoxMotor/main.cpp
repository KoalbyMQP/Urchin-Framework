// New file: ESP32 firmware for the end-to-end bench test
//   laptop/Pi --D-BOX over USB (UART0)--> ESP32 --Herkulex (UART1)--> servo(s)
// Not part of the normal Urchin firmware app -- built only under
// env:DBoxMotor. Combines the two already-verified halves:
//   - D-BOX transport: the same vendored, unmodified D-BOX protocol and
//     Esp32UartPort that env:DBoxEcho proved against the host;
//   - servo control: the same src/BenchShared helpers env:HerkulexTest
//     proved against the servo.
// The host side is Crab_API/src/bench_test.py (D-BOX via DBOXpy, the
// Python bindings of Clam/D-BOX).
//
// v2: several servos on one Herkulex bus (daisy chain).
//
// Addressing (PROVISIONAL -- to be confirmed by the team's stream/ID list):
//   VPID 1..253  = the servo with that Herkulex ID.
//   VPID 0       = the board itself / every known servo at once. With no
//                  servo known yet it falls back to the broadcast ID, so a
//                  v1 host (which always used VPID 0) still works.
//   A servo with Herkulex ID 0 can't be addressed on its own; give it
//   another ID first (SetId never hands out 0).
// Every reply goes back on the VPID the command came in on.
//
// Wire protocol (all payloads are ASCII text):
//   handshake   host -> VPID 0, stream 'S', "Urchin"
//               ESP  -> VPID 0, stream 'S', "Urchin"
//               (mirrors Clam's UartDiscovery identity exchange)
//   commands    host -> stream 'N'; ESP answers each with exactly one
//               stream 'E' packet on the same VPID, "OK ..." or "ERR ...":
//     Info             -> OK DBoxMotor v2 servos=219:0601,253:0201 | servos=none
//     Find             -> OK FOUND 219:0601 253:0201 | OK BLIND     (~8s scan)
//     Status           -> OK STAT <id> <error> <detail> [<id> <error> <detail> ...]
//                         (VPID 0: one triple per known servo, "-" for no
//                         reply) | ERR NOREPLY
//     Spin <speed>     -> OK SPIN <speed>      (-1023..1023, keep-alive)
//     Stop             -> OK STOP
//     Move <deg> <ms>  -> OK MOVE <deg> | ERR RANGE
//     Model <0101|0201|0601|0602>   (servo VPID only) -> OK MODEL <id> <model>
//     SetId <new>      (servo VPID only) -> OK SETID <old> <new>
//                         | ERR TAKEN | ERR BADID | ERR NOREPLY
//   Any command on a servo VPID the board hasn't seen yet first pings that
//   ID; if nothing answers -> ERR NOSERVO <id>.
//   events      ESP  -> stream 'D' (unsolicited), on the servo's VPID:
//                       "AUTOSTOP"
//
// Spin is dead-man controlled, per servo: the host re-sends it every
// ~100ms while the user wants motion; any servo that gets no keep-alive for
// kSpinDeadmanMs is stopped. Closing the laptop program or pulling USB
// therefore always stops every motor.
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
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern bool g_driverLogsEnabled;

static constexpr const char* kIdentity = "Urchin";
static constexpr unsigned long kSpinRefreshMs = 150;
static constexpr unsigned long kSpinDeadmanMs = 500;
static constexpr int kMaxSpeed = 1023;
static constexpr int kMaxServos = 16;
static constexpr uint8_t kBoardVpid = 0;
// Checked at boot (besides one broadcast ping): the factory IDs of the
// DRS-0601 and DRS-0201. Anything else needs a Find.
static constexpr int kFactoryIds[] = {219, 253};

struct Command {
    uint8_t vpid;
    uint8_t stream;
    char text[64];
};

struct Servo {
    int id;
    HerkulexModel model;
    bool modelKnown;  // read from the servo (or set by Model), not assumed
    int spinSpeed;    // 0 = not spinning
    unsigned long lastSpinMsgMs;
    unsigned long lastSpinCmdMs;
};

static QueueHandle_t g_commands;
static DBOXProtocol::DBOXprotocol* g_proto;

static Servo g_servos[kMaxServos];
static int g_servoCount = 0;
// Stand-in used on VPID 0 while no servo is known: talks to the broadcast
// ID, exactly like v1 did.
static Servo g_blind = {Bench::kBroadcastId, Bench::kModel, false, 0, 0, 0};

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
    char buf[256];
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

// --- servo table ------------------------------------------------------------

static Servo* Lookup(int id) {
    for (int i = 0; i < g_servoCount; ++i) {
        if (g_servos[i].id == id) {
            return &g_servos[i];
        }
    }
    return nullptr;
}

// Adds (or refreshes) a servo that just answered, asking it for its model.
static Servo* Register(int id) {
    Servo* s = Lookup(id);
    if (!s) {
        if (g_servoCount == kMaxServos) {
            return nullptr;
        }
        s = &g_servos[g_servoCount++];
        *s = Servo{id, Bench::kModel, false, 0, 0, 0};
        std::sort(g_servos, g_servos + g_servoCount,
                  [](const Servo& a, const Servo& b) { return a.id < b.id; });
        s = Lookup(id);
    }
    HerkulexModel model;
    if (Bench::ReadModel(id, &model)) {
        s->model = model;
        s->modelKnown = true;
    }
    return s;
}

// "219:0601,253:0201" (sep = ',') or "219:0601 253:0201" (sep = ' ').
// A model that was assumed rather than read gets a trailing '?'.
static void ListServos(char* out, size_t size, char sep) {
    out[0] = '\0';
    size_t used = 0;
    for (int i = 0; i < g_servoCount && used < size; ++i) {
        const Servo& s = g_servos[i];
        used += snprintf(out + used, size - used, "%s%d:%s%s", i ? (sep == ',' ? "," : " ") : "",
                         s.id, Bench::InfoFor(s.model).name, s.modelKnown ? "" : "?");
    }
}

// The servos a command on this VPID acts on. Returns how many (0 = the VPID
// names a servo that doesn't answer).
static int Targets(uint8_t vpid, Servo** out) {
    if (vpid == kBoardVpid) {
        if (g_servoCount == 0) {
            out[0] = &g_blind;
            return 1;
        }
        for (int i = 0; i < g_servoCount; ++i) {
            out[i] = &g_servos[i];
        }
        return g_servoCount;
    }
    Servo* s = Lookup(vpid);
    if (!s) {
        Bench::ServoStatus st;
        if (vpid > Bench::kMaxServoId || !Bench::Ping(vpid, &st, nullptr)) {
            return 0;
        }
        s = Register(vpid);
        if (!s) {
            return 0;
        }
    }
    out[0] = s;
    return 1;
}

static void StopServo(Servo* s) {
    Bench::Stop(s->id);
    s->spinSpeed = 0;
}

static void StopAll() {
    for (int i = 0; i < g_servoCount; ++i) {
        if (g_servos[i].spinSpeed != 0) {
            StopServo(&g_servos[i]);
        }
    }
    if (g_blind.spinSpeed != 0) {
        StopServo(&g_blind);
    }
}

static void SpinServo(Servo* s, int speed, unsigned long now) {
    if (speed == 0) {
        if (s->spinSpeed != 0) {
            StopServo(s);
        }
        return;
    }
    if (speed != s->spinSpeed) {
        // New spin (or new speed/direction): re-arm first so it works even
        // if the servo was just powered on.
        Bench::Arm(s->id);
        Bench::SpinAt(s->id, speed, s->model);
        s->spinSpeed = speed;
        s->lastSpinCmdMs = now;
    } else if (now - s->lastSpinCmdMs > kSpinRefreshMs) {
        Bench::SpinAt(s->id, speed, s->model);
        s->lastSpinCmdMs = now;
    }
    s->lastSpinMsgMs = now;
}

// --- commands ---------------------------------------------------------------

static void HandleCommand(const Command& cmd) {
    unsigned long now = millis();
    const char* t = cmd.text;
    uint8_t v = cmd.vpid;
    char list[kMaxServos * 12];

    if (StartsWith(t, "Info")) {
        ListServos(list, sizeof(list), ',');
        Send(v, 'E', "OK DBoxMotor v2 servos=%s", g_servoCount ? list : "none");
        return;
    }
    if (StartsWith(t, "Find")) {
        StopAll();
        int ids[kMaxServos];
        int n = Bench::FindAll(ids, kMaxServos);
        g_servoCount = 0;
        for (int i = 0; i < n; ++i) {
            Register(ids[i]);
        }
        if (n == 0) {
            Bench::Arm(Bench::kBroadcastId);
            Send(v, 'E', "OK BLIND");
        } else {
            ListServos(list, sizeof(list), ' ');
            Send(v, 'E', "OK FOUND %s", list);
        }
        return;
    }
    if (!StartsWith(t, "Status") && !StartsWith(t, "Spin") && !StartsWith(t, "Stop") &&
        !StartsWith(t, "Move") && !StartsWith(t, "Model") && !StartsWith(t, "SetId")) {
        Send(v, 'E', "ERR UNKNOWN %s", t);
        return;
    }

    Servo* targets[kMaxServos];
    int count = Targets(v, targets);
    if (count == 0) {
        Send(v, 'E', "ERR NOSERVO %d", v);
        return;
    }

    if (StartsWith(t, "Status")) {
        char out[kMaxServos * 16];
        size_t used = 0;
        int answered = 0;
        out[0] = '\0';
        for (int i = 0; i < count; ++i) {
            Bench::ServoStatus s;
            int replyId = targets[i]->id;
            if (Bench::Ping(targets[i]->id, &s, &replyId)) {
                used += snprintf(out + used, sizeof(out) - used, " %d %u %u", replyId, s.error,
                                 s.detail);
                ++answered;
            } else {
                used += snprintf(out + used, sizeof(out) - used, " %d - -", targets[i]->id);
            }
        }
        if (answered == 0) {
            Send(v, 'E', "ERR NOREPLY");
        } else {
            Send(v, 'E', "OK STAT%s", out);
        }
    } else if (StartsWith(t, "Spin")) {
        int speed = std::atoi(t + 4);
        speed = std::max(-kMaxSpeed, std::min(kMaxSpeed, speed));
        for (int i = 0; i < count; ++i) {
            SpinServo(targets[i], speed, now);
        }
        Send(v, 'E', "OK SPIN %d", speed);
    } else if (StartsWith(t, "Stop")) {
        for (int i = 0; i < count; ++i) {
            StopServo(targets[i]);
        }
        Send(v, 'E', "OK STOP");
    } else if (StartsWith(t, "Move")) {
        char* end = nullptr;
        float degrees = std::strtof(t + 4, &end);
        int timeMs = (end && *end) ? std::atoi(end) : 1000;
        // HerkulexClass::moveOne silently ignores play times outside
        // 0-2856ms, so clamp rather than reply OK to a move that never
        // happens.
        timeMs = std::max(0, std::min(2856, timeMs));
        if (end == t + 4) {
            Send(v, 'E', "ERR USAGE Move <degrees> <ms>");
            return;
        }
        // Check every target first so a move is all-or-nothing.
        for (int i = 0; i < count; ++i) {
            const Bench::ModelInfo& m = Bench::InfoFor(targets[i]->model);
            int position = static_cast<int>(degrees / m.degPerUnit) + m.centerPos;
            if (position < 0 || position > m.maxPos) {
                Send(v, 'E', "ERR RANGE");
                return;
            }
        }
        for (int i = 0; i < count; ++i) {
            if (targets[i]->spinSpeed != 0) {
                StopServo(targets[i]);
            }
            Bench::Arm(targets[i]->id);
            Bench::MoveToAngle(targets[i]->id, degrees, timeMs, targets[i]->model);
        }
        Send(v, 'E', "OK MOVE %.1f", static_cast<double>(degrees));
    } else if (v == kBoardVpid) {
        // Model / SetId change one servo; refuse them on the "all" VPID.
        Send(v, 'E', "ERR USAGE send %.5s on the servo's VPID", t);
    } else if (StartsWith(t, "Model")) {
        HerkulexModel model;
        const char* arg = t + 5;
        while (*arg == ' ') ++arg;
        if (!Bench::ParseModel(arg, &model)) {
            Send(v, 'E', "ERR USAGE Model <0101|0201|0601|0602>");
            return;
        }
        targets[0]->model = model;
        targets[0]->modelKnown = true;
        Send(v, 'E', "OK MODEL %d %s", targets[0]->id, Bench::InfoFor(model).name);
    } else {  // SetId
        char* end = nullptr;
        long newId = std::strtol(t + 5, &end, 10);
        Servo* s = targets[0];
        Bench::ServoStatus st;
        if (end == t + 5 || newId < 1 || newId > Bench::kMaxServoId) {
            Send(v, 'E', "ERR BADID use 1-%d", Bench::kMaxServoId);
        } else if (newId == s->id) {
            Send(v, 'E', "OK SETID %d %ld", s->id, newId);
        } else if (Lookup(newId) || Bench::Ping(newId, &st, nullptr)) {
            Send(v, 'E', "ERR TAKEN %ld", newId);
        } else {
            int oldId = s->id;
            if (s->spinSpeed != 0) {
                StopServo(s);
            }
            if (Bench::ChangeId(oldId, newId)) {
                s->id = newId;
                std::sort(g_servos, g_servos + g_servoCount,
                          [](const Servo& a, const Servo& b) { return a.id < b.id; });
                Send(v, 'E', "OK SETID %d %ld", oldId, newId);
            } else {
                Send(v, 'E', "ERR NOREPLY");
            }
        }
    }
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

    // Quick look for servos (well under a second) so commands work straight
    // away; the full ~8s scan only runs when the host asks for it with Find.
    // A lone servo answers the broadcast ping with its real ID; with several,
    // their replies collide, so the factory IDs are also tried directly.
    {
        Bench::ServoStatus s;
        int replyId = -1;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (Bench::Ping(Bench::kBroadcastId, &s, &replyId) && replyId <= Bench::kMaxServoId) {
                Register(replyId);
                break;
            }
        }
        for (int id : kFactoryIds) {
            if (!Lookup(id) && Bench::Ping(id, &s, nullptr)) {
                Register(id);
            }
        }
    }

    g_commands = xQueueCreate(8, sizeof(Command));
    xTaskCreate(ReceiveTask, "DBoxRx", 8192, nullptr, 5, nullptr);

    while (true) {
        Command cmd;
        if (xQueueReceive(g_commands, &cmd, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (cmd.stream == 'S') {
                if (std::strcmp(cmd.text, kIdentity) == 0) {
                    Send(kBoardVpid, 'S', "%s", kIdentity);
                }
            } else if (cmd.stream == 'N') {
                HandleCommand(cmd);
            }
            // Any other stream: not for us; ignore.
        }

        // Dead-man, per servo: no Spin keep-alive recently -> stop it.
        unsigned long now = millis();
        for (int i = 0; i <= g_servoCount; ++i) {
            Servo* s = (i < g_servoCount) ? &g_servos[i] : &g_blind;
            if (s->spinSpeed != 0 && now - s->lastSpinMsgMs > kSpinDeadmanMs) {
                StopServo(s);
                // Servo spun via VPID 0 while none was known -> report on 0.
                Send(s == &g_blind ? kBoardVpid : static_cast<uint8_t>(s->id), 'D', "AUTOSTOP");
            }
        }
    }
}
