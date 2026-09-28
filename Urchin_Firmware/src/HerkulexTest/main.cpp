// New file: minimal ESP32 firmware to exercise real Herkulex servo motion
// over UART, using the existing, unmodified src/Herkulex driver. Not part
// of the normal Urchin firmware app -- built only under env:HerkulexTest
// for local hardware testing (see ComsStub.cpp for why this needs a stub).
//
// Behavior: scans IDs 0-253 with Herkulex.stat() to find the real servo ID
// (falling back to broadcast if none respond), then torques it on and hands
// control to the keyboard: hold 'a'/'d' in the serial monitor to jog the
// angle down/up by kJogStepDeg per keypress. Current angle prints after
// each step. Typing relies on the monitor sending raw keystrokes as
// they're typed (true for PlatformIO's default monitor) rather than
// needing Enter.
//
// TODO(you): HERKULEX_UART/TX/RX below are placeholders -- update them to
// match whatever pins you actually wire the servo bus to. This must NOT be
// UART0 (reserved for USB/Pi comms, see ESP_PI_Communication/Coms.h).

#include "../Herkulex/Herkulex.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>

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

// Jog tuning. MODEL_0601's full range is roughly +/-165 deg (see
// Herkulex::moveOneAngle's posLimit/center/degreesPerUnit for that model);
// kJogMaxDeg stays a few degrees inside that so jogging into the limit
// clamps instead of silently failing the driver's out-of-range check.
static constexpr float kJogStepDeg = 10.0f;
static constexpr float kJogMaxDeg = 160.0f;
static constexpr int kJogPlayTime = 20;  // ~224ms per step -- snappy but not abrupt

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

    float angle = 0.0f;
    printf("HerkulexTest: homing to 0.0 deg\n");
    Herkulex.moveOneAngle(servoId, angle, 100, LED_GREEN, SERVO_MODEL, false);

    printf("HerkulexTest: ready -- hold 'a' to jog -%.1f deg/step, 'd' to jog +%.1f deg/step\n",
           kJogStepDeg, kJogStepDeg);

    while (true) {
        int c = getchar();
        if (c == 'a' || c == 'A') {
            angle -= kJogStepDeg;
        } else if (c == 'd' || c == 'D') {
            angle += kJogStepDeg;
        } else {
            continue;  // ignore anything else (newlines, other keys, no data)
        }

        if (angle > kJogMaxDeg) angle = kJogMaxDeg;
        if (angle < -kJogMaxDeg) angle = -kJogMaxDeg;

        Herkulex.moveOneAngle(servoId, angle, kJogPlayTime, LED_BLUE, SERVO_MODEL, false);
        printf("HerkulexTest: angle = %.1f deg\n", angle);
    }
}
