// Shared servo helpers for the bench-test builds (env:HerkulexTest,
// env:DBoxMotor). Not part of the normal Urchin firmware app.
//
// Motion goes through the real src/Herkulex driver. STAT and clear-error
// are sent as local raw packets instead, because the driver's versions
// are buggy:
//   - HerkulexClass::stat() compares the reply's checksum2 against ~ck1
//     without masking bit 0, so every genuine reply fails that check and
//     comes back as {0xFE, 0}; a 9-byte read of line noise also counts as
//     a reply.
//   - HerkulexClass::clearError() declares an 11-byte packet but only
//     fills 10, so the last byte is whatever the previous command left in
//     the buffer; the servo rejects it (bad checksum) once any jog command
//     has run.
// The local versions follow the Herkulex manual's packet layout directly.
//
// None of this prints anything: env:DBoxMotor shares UART0 between the
// console and D-BOX frames, so callers decide what (if anything) to print.

#pragma once

#include "../Herkulex/Herkulex.h"
#include <driver/uart.h>

namespace Bench {

// Wiring -- must match src/HerkulexTest/README.md. Same UART/pins/baud as
// the real firmware (Conversation/UnPacker.cpp's UnpackerInit). Must NOT be
// UART0 (that's the USB console). GPIO16/17 are free on ESP32-WROOM-32 dev
// boards; on WROVER modules they're used by PSRAM.
constexpr uart_port_t kUart = UART_NUM_1;
constexpr int kBaud = 115200;  // Herkulex factory default
constexpr int kTxPin = 17;     // ESP32 TX -> servo RX
constexpr int kRxPin = 16;     // ESP32 RX <- servo TX

constexpr int kBroadcastId = 0xFE;
constexpr int kMaxServoId = 253;  // valid servo IDs are 0..253
// Default model, used when a servo's model is unknown (and by the
// single-servo helpers below that don't take one).
constexpr HerkulexModel kModel = MODEL_0601;
// MODEL_0601's position range, matching HerkulexClass::moveOneAngle's math.
constexpr int kCenterPos = 1024;
constexpr float kDegPerUnit = 0.161f;
constexpr int kMaxPos = 2047;

// Position geometry for each servo family, from the Herkulex manuals (and
// the same numbers HerkulexClass::moveOneAngle uses).
struct ModelInfo {
    HerkulexModel model;
    const char* name;  // "0101", "0201", "0601", "0602"
    int centerPos;     // position value at 0 degrees
    float degPerUnit;
    int maxPos;
};
const ModelInfo& InfoFor(HerkulexModel model);
// Parses "0101" / "0201" / "0601" / "0602" (a leading "DRS-" is allowed).
// Returns false for anything else.
bool ParseModel(const char* text, HerkulexModel* out);

struct ServoStatus {
    uint8_t error;   // STATUS_ERROR register (RAM 48)
    uint8_t detail;  // STATUS_DETAIL register (RAM 49)
};

// Opens UART1 for the servo bus.
void Begin();

// Returns true and fills *out (and *replyId, if given) only for a
// well-formed STAT reply. Use kBroadcastId to ask "whoever is out there".
bool Ping(int id, ServoStatus* out, int* replyId);

// Clears STATUS_ERROR + STATUS_DETAIL. A servo in an error state (red
// blinking LED) turns its torque off and ignores motion until this is sent.
void ClearErrors(int id);

// Makes the servo ready to move no matter what happened before (servo
// powered after the ESP32, brown-out error, torque left off). Cheap enough
// to do before every motion command.
void Arm(int id);

// Reads `len` bytes of the servo's EEPROM starting at `addr` (raw
// EEP_READ). Returns false if no well-formed reply arrived.
bool ReadEep(int id, uint8_t addr, uint8_t len, uint8_t* out);

// Asks the servo what it is (EEPROM "Model No." bytes at address 0-1).
// Returns false if it didn't answer or the number isn't one we know; *out is
// then left alone. Known numbers: 0x01 = DRS-0101, 0x02 = DRS-0201 (both
// from the manuals); 0x04/0x06 = DRS-0401/0601 with second byte 0x02
// meaning the 0x02 (high-resolution) variant -- the 06xx numbers are not yet
// confirmed on real hardware, hence the Model override in DBoxMotor.
bool ReadModel(int id, HerkulexModel* out);

// Every ID 0-253 that answers a STAT, in order (~8s with nothing on the
// bus). No broadcast first: with two or more servos their broadcast replies
// collide. Returns how many were written to ids (at most maxIds).
int FindAll(int* ids, int maxIds);

// Writes a new ID into the servo's EEPROM and reboots it so it takes
// effect. Returns true once the servo answers on the new ID (up to ~2s).
// The caller must make sure newId is free first.
bool ChangeId(int oldId, int newId);

// Returns the servo's ID, or -1 if nothing answered. Tries a broadcast STAT
// first (a lone servo usually answers it with its real ID, instantly), then
// addresses every ID 0-253 (~10s). progress, if given, is called with each
// ID before it's tried.
int FindServo(void (*progress)(int id));

// One continuous-rotation ("speed mode") jog command, sent directly rather
// than via HerkulexClass::moveSpeedOne -- that wrapper blocks for its whole
// totalTime polling the servo, which fights an interactive control loop.
// The servo keeps turning until told otherwise, so callers re-send this
// periodically and call Stop() when done.
void SpinAt(int id, int speed);
void SpinAt(int id, int speed, HerkulexModel model);

void Stop(int id);

// Non-blocking position move (HerkulexClass::moveOneAngle blocks while it
// tracks the motion). Returns false if the angle is outside the servo's
// range.
bool MoveToAngle(int id, float degrees, int timeMs);
bool MoveToAngle(int id, float degrees, int timeMs, HerkulexModel model);

}  // namespace Bench
