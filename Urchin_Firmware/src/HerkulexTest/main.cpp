// New file: minimal ESP32 firmware to exercise real Herkulex servo motion
// over UART, using the existing, unmodified src/Herkulex driver. Not part
// of the normal Urchin firmware app -- built only under env:HerkulexTest
// for local hardware testing (see ComsStub.cpp for why this needs a stub).
//
// Behavior: scans IDs 0-253 with Herkulex.stat() to find the real servo ID
// (falling back to broadcast if none respond), then torques it on and hands
// control to the keyboard: HOLD 'a'/'d' in the serial monitor to spin the
// servo continuously CCW/CW; release to stop. Serial only gives us
// keypress bytes, not up/down events, so "held" is inferred from the
// OS's keyboard auto-repeat -- as long as repeat bytes keep arriving
// faster than kReleaseTimeoutMs apart, we keep re-issuing the spin
// command; once bytes stop arriving, we call it released and stop.
// Typing relies on the monitor sending raw keystrokes as they're typed
// (true for PlatformIO's default monitor) rather than needing Enter.
//
// TODO(you): HERKULEX_UART/TX/RX below are placeholders -- update them to
// match whatever pins you actually wire the servo bus to. This must NOT be
// UART0 (reserved for USB/Pi comms, see ESP_PI_Communication/Coms.h).

#include "../Herkulex/Herkulex.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>
#include <fcntl.h>

// The header only declares `extern HerkulexClass Herkulex;` -- nothing in
// src/ currently defines it (see conversation notes), so this build
// supplies the definition itself.
HerkulexClass Herkulex;

// Plain printf instead of ESP_LOGI: env:common builds with
// LOG_LOCAL_LEVEL=ESP_LOG_NONE, which compiles out ESP_LOGx entirely, and
// redefining that macro per-environment conflicts with -Werror. printf
// goes straight to the UART0 console regardless of ESP-IDF's log level.

// TODO(you): confirm/adjust to match your actual wiring.
#define HERKULEX_UART UART_NUM_1
#define HERKULEX_BAUD 115200
#define HERKULEX_TX_PIN 17
#define HERKULEX_RX_PIN 16

// Fallback if the scan below finds nothing -- broadcast still reaches
// every servo on the bus regardless of its individual ID.
#define BROADCAST_SERVO_ID 0xFE
#define SERVO_MODEL MODEL_0601

// Continuous-rotation tuning. targSpeed is an abstract velocity unit, not
// deg/s -- HerkulexClass::moveSpeedOne's own (commented-out) validation
// treats +/-1023 as the sane range, so this stays well inside that.
static constexpr int kSpinSpeed = 200;
// Each spin command covers this many ms before the servo would stop on
// its own; refreshed well before it lapses as long as the key is held, so
// motion never stutters.
static constexpr int kSpinCommandMs = 300;
static constexpr unsigned long kRefreshIntervalMs = 150;
// No repeat keystroke within this long => treat the key as released.
static constexpr unsigned long kReleaseTimeoutMs = 200;
static constexpr TickType_t kPollInterval = pdMS_TO_TICKS(20);

// Herkulex IDs run 0-253; 0xFE is broadcast and 0xFF is reserved, so
// neither is a real servo address. Only the servo actually present at a
// given ID replies to stat() -- every other ID just times out (~50ms/id,
// see Herkulex::readData), so a full scan takes a few seconds.
static int FindServoId() {
    for (int id = 0; id < BROADCAST_SERVO_ID; ++id) {
        if (Herkulex.stat(id).has_value()) {
            return id;
        }
    }
    return -1;
}

// Sends one continuous-rotation ("speed mode") jog command directly,
// bypassing HerkulexClass::moveSpeedOne -- that wrapper blocks for its
// entire totalTime polling the servo's RAM tick register, which fights a
// keyboard-driven control loop that needs to react immediately to
// keypresses/releases. LSB/MSB sign-magnitude encoding mirrors what
// moveSpeedOne does internally before its blocking loop.
static void SpinAt(int servoID, int targSpeed, int pTimeMs, JogLedColor led, HerkulexModel model) {
    int magnitude = (targSpeed < 0) ? ((-targSpeed) | 0x4000) : targSpeed;
    int lsb = magnitude & 0x00FF;
    int msb = (magnitude & 0xFF00) >> 8;
    Herkulex.sendSJog(servoID, targSpeed, pTimeMs, led, model, /*Mode=*/true, lsb, msb);
}

extern "C" void app_main() {
    Herkulex.begin(HERKULEX_UART, HERKULEX_BAUD, HERKULEX_RX_PIN, HERKULEX_TX_PIN);
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("HerkulexTest: scanning IDs 0-253 for a responding servo...\n");
    int servoId = FindServoId();
    if (servoId >= 0) {
        printf("HerkulexTest: found servo at ID %d\n", servoId);
    } else {
        printf("HerkulexTest: no servo responded to the scan; falling back to broadcast (0x%02X)\n",
               BROADCAST_SERVO_ID);
        servoId = BROADCAST_SERVO_ID;
    }

    Herkulex.clearError(servoId);
    Herkulex.torqueON(servoId);

    // Non-blocking stdin: getchar() returns immediately (EOF) when no
    // keystroke is waiting, instead of blocking, so the loop below can
    // also track elapsed time for the release timeout.
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    printf("HerkulexTest: ready -- hold 'a' to spin CCW, 'd' to spin CW, release to stop\n");

    int direction = 0;          // -1 = CCW ('a'), 0 = stopped, +1 = CW ('d')
    int lastPrintedDirection = 0;
    unsigned long lastKeyMs = 0;
    unsigned long lastCommandMs = 0;

    while (true) {
        int c = getchar();
        unsigned long now = millis();

        if (c == 'a' || c == 'A') {
            direction = -1;
            lastKeyMs = now;
        } else if (c == 'd' || c == 'D') {
            direction = 1;
            lastKeyMs = now;
        }

        if (direction != 0 && (now - lastKeyMs) > kReleaseTimeoutMs) {
            Herkulex.motor_stop(servoId);
            printf("HerkulexTest: stopped\n");
            direction = 0;
            lastPrintedDirection = 0;
        } else if (direction != 0 && (now - lastCommandMs) > kRefreshIntervalMs) {
            SpinAt(servoId, direction * kSpinSpeed, kSpinCommandMs,
                   direction < 0 ? LED_RED : LED_BLUE, SERVO_MODEL);
            lastCommandMs = now;
            if (direction != lastPrintedDirection) {
                printf("HerkulexTest: spinning %s\n", direction < 0 ? "CCW" : "CW");
                lastPrintedDirection = direction;
            }
        }

        vTaskDelay(kPollInterval);
    }
}
