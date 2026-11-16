// D-BOX link between the Pi and the ESP32 (UART0).
// Only used by the Urchin-Firmware-DBOX build (URCHIN_DBOX defined);
// the normal build keeps the old fixed 1027-byte Box.
#ifndef DBOXLINK_H
#define DBOXLINK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Wraps the already-configured UART0 in a D-BOX protocol object. Call once, after SetUpUart().
int DboxLinkInit(void);

// Sends one packet: exactly `size` bytes of payload, no padding. Returns 0 on success.
int DboxSend(uint8_t vpid, char stream, const uint8_t *data, size_t size);

// Waits for the next packet. Copies at most `capacity` payload bytes into `data`,
// zero-fills the rest, and returns 1 (0 if the link isn't set up).
int DboxReceive(uint8_t *vpid, uint8_t *stream, uint8_t *data, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif // DBOXLINK_H
