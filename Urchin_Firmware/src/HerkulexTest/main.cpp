// New file: minimal ESP32 firmware to exercise real Herkulex servo motion
// over UART, using the existing src/Herkulex driver for all motion
// commands. Not part of the normal Urchin firmware app -- built only under
// env:HerkulexTest for local hardware testing. Servo helpers live in
// src/BenchShared (shared with env:DBoxMotor). Setup guide for people
// running it: src/HerkulexTest/README.md
//
// Design goal: give the same result every time, regardless of the order
// things get powered on/plugged in. Concretely:
//   - The ESP32 can boot before or after the servo's DC supply is on. We
//     never rely on a one-time init at boot: clear-errors + torque-on are
//     re-sent right before every spin starts, and a periodic health check
//     notices the servo appearing/disappearing and says so in plain words.
//   - If the servo never answers (e.g. its TX -> ESP32 RX wire is the
//     broken one), we still drive it in "blind" mode over the broadcast ID,
//     so motion can still be checked; the console says this clearly.
//   - Status replies are validated (header, ID, both checksums) after
//     flushing stale RX bytes, so line noise can't masquerade as a servo.
//
// Controls (serial monitor, 115200): HOLD 'a'/'d' to spin CCW/CW, release
// to stop. 's' = status, 'r' = re-scan for the servo, 'h' = help.
//
// Serial only gives us keypress bytes, not up/down events, so "held" is
// inferred from the OS's keyboard auto-repeat. Windows waits ~500ms (up to
// 1s depending on settings) after the first byte before auto-repeat kicks
// in, so the first byte of a hold gets a longer grace period than the
// gaps between repeats -- otherwise a held key would spin, stop, spin.

#include "../BenchShared/ServoBench.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>
#include <fcntl.h>

using Bench::ServoStatus;

// From BenchShared/ComsStub.cpp: the driver's own debug prints (e.g.
// motor_stop dumps raw checksum numbers) are noise for this test, so
// they're muted.
extern bool g_driverLogsEnabled;

// Plain printf instead of ESP_LOGI: env:common builds with
// LOG_LOCAL_LEVEL=ESP_LOG_NONE, which compiles out ESP_LOGx entirely, and
// redefining that macro per-environment conflicts with -Werror.

static constexpr const char* kFirmwareBanner = "HerkulexTest firmware v2";

// Continuous-rotation tuning. Speed is an abstract velocity unit, not
// deg/s -- HerkulexClass::moveSpeedOne's own (commented-out) validation
// treats +/-1023 as the sane range, so this stays well inside that.
static constexpr int kSpinSpeed = 200;
static constexpr unsigned long kRefreshIntervalMs = 150;
// Release detection: see the auto-repeat note at the top of the file.
static constexpr unsigned long kFirstRepeatGraceMs = 1100;
static constexpr unsigned long kReleaseTimeoutMs = 250;
static constexpr unsigned long kHealthCheckIntervalMs = 2000;
static constexpr TickType_t kPollInterval = pdMS_TO_TICKS(20);

// --- Human-readable status --------------------------------------------------

static void PrintStatus(int id, const ServoStatus& s) {
    printf("  servo ID %d: ", id);
    if (s.error == 0) {
        printf("no errors");
    }
    if (s.error & 0x01) printf("[INPUT VOLTAGE OUT OF RANGE - check DC supply voltage/current limit] ");
    if (s.error & 0x02) printf("[POSITION LIMIT exceeded] ");
    if (s.error & 0x04) printf("[OVERHEATED - power off and let it cool] ");
    if (s.error & 0x08) printf("[BAD PACKET received - check wiring/ground/noise] ");
    if (s.error & 0x10) printf("[OVERLOAD - shaft blocked or stalled] ");
    if (s.error & 0x40) printf("[EEPROM corrupted - needs Herkulex Manager] ");
    printf(", torque %s\n", (s.detail & 0x40) ? "ON" : "OFF");
}

static void PrintHelp() {
    printf("\n  HOLD 'a' = spin CCW   HOLD 'd' = spin CW   (let go to stop)\n");
    printf("  's' = servo status   'r' = re-scan for servo   'h' = this help\n\n");
}

// --- Servo discovery ------------------------------------------------------

static void ScanProgress(int id) {
    if (id == 0) {
        printf("HerkulexTest: scanning IDs 0-253 (about 10 seconds)");
    }
    if (id % 32 == 0) {
        printf(".");
        fflush(stdout);
    }
}

static int Discover(bool* blind) {
    printf("HerkulexTest: looking for the servo...\n");
    int id = Bench::FindServo(ScanProgress);
    printf("\n");
    ServoStatus s;
    if (id >= 0 && Bench::Ping(id, &s, nullptr)) {
        *blind = false;
        printf("HerkulexTest: FOUND servo -- two-way communication OK\n");
        PrintStatus(id, s);
        return id;
    }
    *blind = true;
    printf("HerkulexTest: no reply from any servo.\n");
    printf("  -> If the servo is not powered yet, that's fine: power it on and press 'r'.\n");
    printf("  -> Continuing in BLIND mode (broadcast ID). If 'a'/'d' still spin it, the\n");
    printf("     ESP32->servo wire works but the servo->ESP32 wire (servo TX -> GPIO%d) does not.\n",
           Bench::kRxPin);
    return Bench::kBroadcastId;
}

extern "C" void app_main() {
    g_driverLogsEnabled = false;

    printf("\n==== %s ====\n", kFirmwareBanner);
    printf("UART%d  TX=GPIO%d  RX=GPIO%d  %d baud\n", Bench::kUart, Bench::kTxPin, Bench::kRxPin, Bench::kBaud);

    Bench::Begin();
    vTaskDelay(pdMS_TO_TICKS(200));

    bool blind = true;
    int servoId = Discover(&blind);
    Bench::Arm(servoId);

    // Non-blocking stdin: getchar() returns EOF immediately when no
    // keystroke is waiting, so the loop can also track time.
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    printf("HerkulexTest: READY");
    PrintHelp();

    int direction = 0;  // -1 = CCW ('a'), 0 = stopped, +1 = CW ('d')
    bool sawRepeat = false;
    unsigned long lastKeyMs = 0;
    unsigned long lastCommandMs = 0;
    unsigned long lastHealthMs = millis();
    bool servoWasAnswering = !blind;

    while (true) {
        unsigned long now = millis();

        // Drain every byte that arrived since the last pass.
        int c;
        while ((c = getchar()) != EOF) {
            int pressed = 0;
            if (c == 'a' || c == 'A') {
                pressed = -1;
            } else if (c == 'd' || c == 'D') {
                pressed = 1;
            } else if (c == 's' || c == 'S') {
                ServoStatus s;
                if (Bench::Ping(servoId, &s, nullptr)) {
                    PrintStatus(servoId, s);
                } else {
                    printf("HerkulexTest: servo is not answering (%s)\n",
                           blind ? "blind mode" : "was answering before -- power/wiring?");
                }
            } else if (c == 'r' || c == 'R') {
                if (direction != 0) {
                    Bench::Stop(servoId);
                    direction = 0;
                }
                servoId = Discover(&blind);
                Bench::Arm(servoId);
                servoWasAnswering = !blind;
                printf("HerkulexTest: READY\n");
            } else if (c == 'h' || c == 'H' || c == '?') {
                PrintHelp();
            }

            if (pressed != 0) {
                if (pressed == direction) {
                    sawRepeat = true;
                } else {
                    // New hold (or direction change): re-arm first so it
                    // works even if the servo was just powered on.
                    Bench::Arm(servoId);
                    direction = pressed;
                    sawRepeat = false;
                    lastCommandMs = 0;
                    printf("HerkulexTest: spinning %s\n", direction < 0 ? "CCW" : "CW");
                }
                lastKeyMs = now;
            }
        }

        if (direction != 0) {
            unsigned long timeout = sawRepeat ? kReleaseTimeoutMs : kFirstRepeatGraceMs;
            if (now - lastKeyMs > timeout) {
                Bench::Stop(servoId);
                printf("HerkulexTest: stopped\n");
                direction = 0;
            } else if (lastCommandMs == 0 || now - lastCommandMs > kRefreshIntervalMs) {
                Bench::SpinAt(servoId, direction * kSpinSpeed);
                lastCommandMs = now;
            }
        } else if (now - lastHealthMs > kHealthCheckIntervalMs) {
            // Idle health check: report the servo appearing/disappearing so
            // a loose wire or a supply that got switched off is obvious.
            lastHealthMs = now;
            ServoStatus s;
            int replyId = -1;
            bool answering = blind ? Bench::Ping(Bench::kBroadcastId, &s, &replyId)
                                   : Bench::Ping(servoId, &s, nullptr);
            if (answering && !servoWasAnswering) {
                printf("HerkulexTest: servo is answering now -- press 'r' to switch out of blind mode\n");
                if (!blind) {
                    PrintStatus(servoId, s);
                }
            } else if (!answering && servoWasAnswering && !blind) {
                printf("HerkulexTest: LOST the servo -- check DC supply and wiring, then press 'r'\n");
            } else if (answering && s.error != 0 && !blind) {
                printf("HerkulexTest: servo reports an error (will auto-clear on next spin):\n");
                PrintStatus(servoId, s);
            }
            servoWasAnswering = answering;
        }

        vTaskDelay(kPollInterval);
    }
}
