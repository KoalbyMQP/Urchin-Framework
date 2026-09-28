// New file (test-only stub): minimal stand-ins for the symbols the real,
// unmodified src/Herkulex driver expects from the normal Urchin app
// (PackfToPI/PrintfToPI + the debug queue handle), so this isolated build
// doesn't have to pull in the full ESP_PI_Communication/Shipping/Bridge
// stack just to link. Local-only, not part of the normal Urchin firmware
// app -- built only under env:HerkulexTest.
//
// Herkulex.cpp only calls PrintfToPI(DebugQueue, ...) for debug logging;
// here that just goes straight to the console instead of through a queue.

#include "../ESP_PI_Communication/Coms.h"
#include "../ESP_PI_Communication/MSGQueue.h"
#include <cstdarg>
#include <cstdio>

QueueHandle_t ExchangeQueue = nullptr;
QueueHandle_t RecationQueue = nullptr;
QueueHandle_t DebugQueue = nullptr;

extern "C" int PackfToPI(QueueHandle_t, const uint8_t, const char buff[], size_t buff_size) {
    printf("%.*s", static_cast<int>(buff_size), buff);
    return Success;
}

extern "C" int PrintfToPI(QueueHandle_t, const uint8_t, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}
