// New file: ESP32 firmware for the end-to-end bench test
//   laptop/Pi --D-BOX over USB (UART0)--> ESP32 --Herkulex (UART1, UART2)--> servo(s)
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
// v3: - a second servo bus (UART2, GPIO26 TX / GPIO27 RX). Servo families
//       that can't share one reply line (a DRS-0601 with a DRS-0201/0101 on
//       our bench) go on separate buses. Find scans both; every servo
//       remembers its bus, so commands, VPIDs and the bench test treat the
//       two buses as one motor list. Servo IDs must still be unique across
//       both buses (the first one found keeps the ID).
//     - stream 'M' (PROVISIONAL letter): the payload is a MarshalType Item
//       (Clam/MarshalType), the team's binary command format, instead of
//       text. joint = the servo ID as text ("10") or "all"; command and
//       values = the text command's words, e.g. Item("10", "Move",
//       [30.0, 1000]) does exactly what "Move 30 1000" on VPID 10 does, and
//       answers the same way (stream 'E', on that servo's VPID).
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
//     Info             -> OK DBoxMotor v3 servos=6:0601@2,10:0201 | servos=none
//                         ("@2" = on bus 2; no suffix = bus 1)
//     Find             -> OK FOUND 6:0601@2 10:0201 | OK BLIND     (~16s, both buses)
//     Status           -> OK STAT <id> <error> <detail> [<id> <error> <detail> ...]
//                         (VPID 0: one triple per known servo, "-" for no
//                         reply) | ERR NOREPLY
//     Spin <speed>     -> OK SPIN <speed>      (-1023..1023, keep-alive)
//     Stop             -> OK STOP
//     Move <deg> <ms>  -> OK MOVE <deg> | ERR RANGE
//     Model <0101|0201|0601|0602>   (servo VPID only) -> OK MODEL <id> <model>
//     SetId <new>      (servo VPID only) -> OK SETID <old> <new>
//                         | ERR TAKEN | ERR BADID | ERR NOREPLY
//     BaudScan         (VPID 0, ONE servo per bus) -> OK BAUD <id> <baud> <bus>
//                         | OK BAUD NONE      finds a servo set to another speed
//     FixBaud <id> <baud> [bus]  (VPID 0) -> OK FIXBAUD <id> | ERR NOREPLY
//                         sets that servo back to 115200 (EEPROM + reboot)
//     Sniff [id] [bus]  (VPID 0; default id = broadcast, bus 1) -> OK RAW idle=<n> got=<n> <hex bytes>
//                         raw bytes on the servo reply line after a STAT,
//                         unchecked (debug aid; Crab_API/src/servo_sniff.py)
//   synchronized moves (queue targets, then start them all on one tick):
//     Queue <deg> [ms] (servo VPID, or VPID 0 = every known servo)
//                      -> OK QUEUED <id> <queued> | ERR RANGE
//                         [ms] gives that servo its own travel time
//     Go [ms]          (VPID 0) -> OK GO <n> SJOG | OK GO <n> IJOG | ERR EMPTY
//                         no own times queued: one S_JOG packet per bus,
//                         all start AND arrive together after [ms] (1000);
//                         any own time queued: I_JOG, all start together,
//                         each arrives after its own time ([ms] for the rest)
//     Clear            (VPID 0) -> OK CLEARED      drops the queue
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
#include <Item.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

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
    uint16_t len;      // payload bytes in text (binary for stream 'M')
    bool truncated;    // payload was longer than text holds
    char text[160];    // always NUL-terminated after len bytes
};

struct Servo {
    int id;
    HerkulexModel model;
    bool modelKnown;  // read from the servo (or set by Model), not assumed
    int spinSpeed;    // 0 = not spinning
    unsigned long lastSpinMsgMs;
    unsigned long lastSpinCmdMs;
    int bus;          // 1 or 2 (Bench::SelectBus)
};

// Targets waiting for Go (synchronized moves).
struct Queued {
    int id;          // servo ID (looked up again on Go)
    int position;    // raw servo units
    int timeMs;      // own travel time, or -1 for "use Go's time"
};
static Queued g_queue[kMaxServos];
static int g_queueCount = 0;

static QueueHandle_t g_commands;
static DBOXProtocol::DBOXprotocol* g_proto;

static Servo g_servos[kMaxServos];
static int g_servoCount = 0;
// Stand-ins used on VPID 0 while no servo is known: talk to the broadcast
// ID on each bus, exactly like v1 did on its one bus.
static Servo g_blind[Bench::kBusCount] = {
    {Bench::kBroadcastId, Bench::kModel, false, 0, 0, 0, 1},
    {Bench::kBroadcastId, Bench::kModel, false, 0, 0, 0, 2},
};

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
        cmd.len = static_cast<uint16_t>(n);
        cmd.truncated = incoming.Payload.size() > n;
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

// Points the servo helpers at this servo's bus.
static void Use(const Servo* s) {
    Bench::SelectBus(s->bus);
}

// Adds (or refreshes) a servo that just answered on `bus`, asking it for its
// model. An ID already registered on the other bus keeps its first bus.
static Servo* Register(int id, int bus) {
    Servo* s = Lookup(id);
    if (s && s->bus != bus) {
        return s;
    }
    if (!s) {
        if (g_servoCount == kMaxServos) {
            return nullptr;
        }
        s = &g_servos[g_servoCount++];
        *s = Servo{id, Bench::kModel, false, 0, 0, 0, bus};
        std::sort(g_servos, g_servos + g_servoCount,
                  [](const Servo& a, const Servo& b) { return a.id < b.id; });
        s = Lookup(id);
    }
    HerkulexModel model;
    Use(s);
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
        used += snprintf(out + used, size - used, "%s%d:%s%s%s", i ? (sep == ',' ? "," : " ") : "",
                         s.id, Bench::InfoFor(s.model).name, s.modelKnown ? "" : "?",
                         s.bus == 2 ? "@2" : "");
    }
}

// The servos a command on this VPID acts on. Returns how many (0 = the VPID
// names a servo that doesn't answer).
static int Targets(uint8_t vpid, Servo** out) {
    if (vpid == kBoardVpid) {
        if (g_servoCount == 0) {
            for (int b = 0; b < Bench::kBusCount; ++b) {
                out[b] = &g_blind[b];
            }
            return Bench::kBusCount;
        }
        for (int i = 0; i < g_servoCount; ++i) {
            out[i] = &g_servos[i];
        }
        return g_servoCount;
    }
    Servo* s = Lookup(vpid);
    if (!s) {
        if (vpid > Bench::kMaxServoId) {
            return 0;
        }
        Bench::ServoStatus st;
        for (int bus = 1; bus <= Bench::kBusCount && !s; ++bus) {
            Bench::SelectBus(bus);
            if (Bench::Ping(vpid, &st, nullptr)) {
                s = Register(vpid, bus);
            }
        }
        if (!s) {
            return 0;
        }
    }
    out[0] = s;
    return 1;
}

static void StopServo(Servo* s) {
    Use(s);
    Bench::Stop(s->id);
    s->spinSpeed = 0;
}

static void StopAll() {
    for (int i = 0; i < g_servoCount; ++i) {
        if (g_servos[i].spinSpeed != 0) {
            StopServo(&g_servos[i]);
        }
    }
    for (Servo& b : g_blind) {
        if (b.spinSpeed != 0) {
            StopServo(&b);
        }
    }
}

static void SpinServo(Servo* s, int speed, unsigned long now) {
    if (speed == 0) {
        if (s->spinSpeed != 0) {
            StopServo(s);
        }
        return;
    }
    Use(s);
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
        Send(v, 'E', "OK DBoxMotor v3 servos=%s", g_servoCount ? list : "none");
        return;
    }
    if (StartsWith(t, "Find")) {
        StopAll();
        g_servoCount = 0;
        int n = 0;
        for (int bus = 1; bus <= Bench::kBusCount; ++bus) {
            int ids[kMaxServos];
            Bench::SelectBus(bus);
            int found = Bench::FindAll(ids, kMaxServos - g_servoCount);
            for (int i = 0; i < found; ++i) {
                Register(ids[i], bus);
            }
            n += found;
        }
        if (n == 0) {
            for (int bus = 1; bus <= Bench::kBusCount; ++bus) {
                Bench::SelectBus(bus);
                Bench::Arm(Bench::kBroadcastId);
            }
            Send(v, 'E', "OK BLIND");
        } else {
            ListServos(list, sizeof(list), ' ');
            Send(v, 'E', "OK FOUND %s", list);
        }
        return;
    }
    if (StartsWith(t, "BaudScan")) {
        StopAll();
        int id = -1;
        int baud = -1;
        int bus = 1;
        for (; bus <= Bench::kBusCount && baud < 0; ++bus) {
            Bench::SelectBus(bus);
            baud = Bench::ScanBauds(&id);
        }
        if (baud < 0) {
            Send(v, 'E', "OK BAUD NONE");
        } else {
            Send(v, 'E', "OK BAUD %d %d %d", id, baud, bus - 1);
        }
        return;
    }
    if (StartsWith(t, "Sniff")) {
        StopAll();
        char* end = nullptr;
        long id = std::strtol(t + 5, &end, 10);
        if (end == t + 5 || id < 0 || id > Bench::kBroadcastId) {
            id = Bench::kBroadcastId;
        }
        long bus = (end && *end) ? std::strtol(end, nullptr, 10) : 1;
        Bench::SelectBus(static_cast<int>(bus));
        uint8_t raw[32];
        int idle = 0;
        int got = Bench::SniffStat(static_cast<int>(id), raw, sizeof(raw), &idle);
        char hex[sizeof(raw) * 3 + 1];
        hex[0] = '\0';
        for (int i = 0; i < got; ++i) {
            snprintf(hex + i * 3, 4, " %02X", raw[i]);
        }
        Send(v, 'E', "OK RAW idle=%d got=%d%s", idle, got, hex);
        return;
    }
    if (StartsWith(t, "FixBaud")) {
        char* end = nullptr;
        long id = std::strtol(t + 7, &end, 10);
        char* end2 = nullptr;
        long baud = (end && *end) ? std::strtol(end, &end2, 10) : 0;
        long bus = (end2 && *end2) ? std::strtol(end2, nullptr, 10) : 1;
        Bench::SelectBus(static_cast<int>(bus));
        if (end == t + 7 || id < 0 || id > Bench::kMaxServoId || baud <= 0) {
            Send(v, 'E', "ERR USAGE FixBaud <id> <baud> [bus]");
        } else if (Bench::ResetBaud(id, baud)) {
            Register(id, Bench::SelectedBus());
            Send(v, 'E', "OK FIXBAUD %ld", id);
        } else {
            Send(v, 'E', "ERR NOREPLY");
        }
        return;
    }
    if (StartsWith(t, "Clear")) {
        g_queueCount = 0;
        Send(v, 'E', "OK CLEARED");
        return;
    }
    if (StartsWith(t, "Go")) {
        if (g_queueCount == 0) {
            Send(v, 'E', "ERR EMPTY queue targets with Queue first");
            return;
        }
        int sharedMs = (t[2] == ' ') ? std::atoi(t + 3) : 1000;
        sharedMs = std::max(0, std::min(2856, sharedMs));
        bool eachOwnTime = false;
        for (int i = 0; i < g_queueCount; ++i) {
            eachOwnTime = eachOwnTime || g_queue[i].timeMs >= 0;
        }
        // Arm every target first, so the one packet per bus starts them all
        // on the same tick (arming takes a few ms per servo).
        Bench::JogTarget jt[Bench::kBusCount + 1][Bench::kMaxJogTargets];
        int n[Bench::kBusCount + 1] = {};
        for (int i = 0; i < g_queueCount; ++i) {
            Servo* s = Lookup(g_queue[i].id);
            if (!s || s->bus < 1 || s->bus > Bench::kBusCount ||
                n[s->bus] == Bench::kMaxJogTargets) {
                continue;
            }
            if (s->spinSpeed != 0) {
                StopServo(s);
            }
            Use(s);
            Bench::Arm(s->id);
            int own = g_queue[i].timeMs;
            jt[s->bus][n[s->bus]++] =
                Bench::JogTarget{s->id, g_queue[i].position, own >= 0 ? own : sharedMs};
        }
        // Then the packets back to back (one per bus; on one bus that's one
        // packet for everybody).
        int sent = 0;
        for (int bus = 1; bus <= Bench::kBusCount; ++bus) {
            if (n[bus] == 0) {
                continue;
            }
            Bench::SelectBus(bus);
            if (eachOwnTime) {
                Bench::SyncMoveEach(jt[bus], n[bus]);
            } else {
                Bench::SyncMove(jt[bus], n[bus], sharedMs);
            }
            sent += n[bus];
        }
        g_queueCount = 0;
        Send(v, 'E', "OK GO %d %s", sent, eachOwnTime ? "IJOG" : "SJOG");
        return;
    }
    if (!StartsWith(t, "Status") && !StartsWith(t, "Spin") && !StartsWith(t, "Stop") &&
        !StartsWith(t, "Move") && !StartsWith(t, "Model") && !StartsWith(t, "SetId") &&
        !StartsWith(t, "Queue")) {
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
            Use(targets[i]);
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
            Use(targets[i]);
            Bench::Arm(targets[i]->id);
            Bench::MoveToAngle(targets[i]->id, degrees, timeMs, targets[i]->model);
        }
        Send(v, 'E', "OK MOVE %.1f", static_cast<double>(degrees));
    } else if (StartsWith(t, "Queue")) {
        char* end = nullptr;
        float degrees = std::strtof(t + 5, &end);
        if (end == t + 5) {
            Send(v, 'E', "ERR USAGE Queue <degrees> [ms]");
            return;
        }
        int ownMs = (end && *end) ? std::atoi(end) : -1;
        if (ownMs >= 0) {
            ownMs = std::min(2856, ownMs);
        }
        // All-or-nothing, like Move.
        int positions[kMaxServos];
        for (int i = 0; i < count; ++i) {
            if (!Bench::PositionFor(degrees, targets[i]->model, &positions[i])) {
                Send(v, 'E', "ERR RANGE");
                return;
            }
        }
        for (int i = 0; i < count; ++i) {
            int slot = 0;
            while (slot < g_queueCount && g_queue[slot].id != targets[i]->id) {
                ++slot;
            }
            if (slot == g_queueCount) {
                if (g_queueCount == kMaxServos) {
                    Send(v, 'E', "ERR FULL");
                    return;
                }
                ++g_queueCount;
            }
            g_queue[slot] = Queued{targets[i]->id, positions[i], ownMs};
        }
        Send(v, 'E', "OK QUEUED %d %d", count == 1 ? targets[0]->id : 0, g_queueCount);
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
        Use(s);
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
            Use(s);
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

// Stream 'M': the payload is a MarshalType Item (Clam/MarshalType). It is
// turned into the equivalent text command and run through HandleCommand, so
// both formats behave (and answer) identically:
//   Item(joint "10", command "Move", values [30.0, 1000])  ==  "Move 30 1000" on VPID 10
//   joint "all" (or "0")                                     ==  VPID 0, every known servo
// In the real firmware the joint would be a name looked up in the bridge
// list; the bench has no bridge, so it uses the servo ID.
static void HandleItem(const Command& cmd) {
    uint8_t v = cmd.vpid;
    if (cmd.truncated) {
        Send(v, 'E', "ERR ITEM TOOLONG");
        return;
    }
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(cmd.text);
    std::vector<uint8_t> bytes(raw, raw + cmd.len);
    MarshalType::Item item;
    if (item.DeMarshal(bytes) != 0) {
        Send(v, 'E', "ERR ITEM BADFORMAT");
        return;
    }

    uint8_t target = kBoardVpid;
    if (item.joint != "all") {
        char* end = nullptr;
        long id = std::strtol(item.joint.c_str(), &end, 10);
        if (item.joint.empty() || *end != '\0' || id < 0 || id > Bench::kMaxServoId) {
            Send(v, 'E', "ERR ITEM JOINT %s", item.joint.c_str());
            return;
        }
        target = static_cast<uint8_t>(id);
    }

    Command text{};
    text.vpid = target;
    text.stream = 'N';
    size_t used = snprintf(text.text, sizeof(text.text), "%s", item.command.c_str());
    for (const MarshalType::ValVariant& value : item.values) {
        if (used >= sizeof(text.text)) {
            break;
        }
        std::visit(
            [&](const auto& x) {
                using T = std::decay_t<decltype(x)>;
                char* out = text.text + used;
                size_t room = sizeof(text.text) - used;
                if constexpr (std::is_same_v<T, MarshalType::bool32_t>) {
                    used += snprintf(out, room, " %d", x.v ? 1 : 0);
                } else if constexpr (std::is_same_v<T, MarshalType::float32_t>) {
                    used += snprintf(out, room, " %g", static_cast<double>(x));
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    used += snprintf(out, room, " %ld", static_cast<long>(x));
                } else {
                    used += snprintf(out, room, " %lu", static_cast<unsigned long>(x));
                }
            },
            value.get());
    }
    if (used >= sizeof(text.text)) {
        Send(v, 'E', "ERR ITEM TOOLONG");
        return;
    }
    text.len = static_cast<uint16_t>(used);
    HandleCommand(text);
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
    Bench::BeginBus2();
    vTaskDelay(pdMS_TO_TICKS(200));

    // Quick look for servos (well under a second) so commands work straight
    // away; the full ~8s scan only runs when the host asks for it with Find.
    // A lone servo answers the broadcast ping with its real ID; with several,
    // their replies collide, so the factory IDs are also tried directly.
    for (int bus = 1; bus <= Bench::kBusCount; ++bus) {
        Bench::SelectBus(bus);
        Bench::ServoStatus s;
        int replyId = -1;
        for (int attempt = 0; attempt < 3; ++attempt) {
            if (Bench::Ping(Bench::kBroadcastId, &s, &replyId) && replyId <= Bench::kMaxServoId) {
                Register(replyId, bus);
                break;
            }
        }
        for (int id : kFactoryIds) {
            Bench::SelectBus(bus);
            if (!Lookup(id) && Bench::Ping(id, &s, nullptr)) {
                Register(id, bus);
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
            } else if (cmd.stream == 'M') {
                HandleItem(cmd);
            }
            // Any other stream: not for us; ignore.
        }

        // Dead-man, per servo: no Spin keep-alive recently -> stop it.
        unsigned long now = millis();
        for (int i = 0; i < g_servoCount + Bench::kBusCount; ++i) {
            bool blind = i >= g_servoCount;
            Servo* s = blind ? &g_blind[i - g_servoCount] : &g_servos[i];
            if (s->spinSpeed != 0 && now - s->lastSpinMsgMs > kSpinDeadmanMs) {
                StopServo(s);
                // Servo spun via VPID 0 while none was known -> report on 0.
                Send(blind ? kBoardVpid : static_cast<uint8_t>(s->id), 'D', "AUTOSTOP");
            }
        }
    }
}
