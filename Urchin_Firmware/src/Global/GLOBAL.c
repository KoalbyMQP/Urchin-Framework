//
// Created by gabri on 5/26/2025.
//
#include "GLOBAL.h"

#include <driver/gpio.h>
// #include <soc/gpio_num.h>
//
//
// void DoSomething() {
//     gpio_set_level((gpio_num_t)CherpPin, 1);
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
//     gpio_set_level((gpio_num_t)CherpPin, 0);
//
// }

void unsignedIntToBytes(unsigned int value, unsigned char* byteArray) {
    if (byteArray == NULL) return;

    byteArray[0] = (value >> 24) & 0xFF; // Most significant byte
    byteArray[1] = (value >> 16) & 0xFF;
    byteArray[2] = (value >> 8) & 0xFF;
    byteArray[3] = value & 0xFF;         // Least significant byte
}


unsigned int BytesToUnsignedInt(const unsigned char* byteArray) {
    if (byteArray == NULL) return 0;  // Safety check

    unsigned int value = 0;
    value |= ((unsigned int)byteArray[0] << 24);
    value |= ((unsigned int)byteArray[1] << 16);
    value |= ((unsigned int)byteArray[2] << 8);
    value |= ((unsigned int)byteArray[3]);

    return value;
}

int BytesToInt(const unsigned char* byteArray) {
    if (byteArray == NULL) return 0;  // Safety check

     int value = 0;
    value |= ((int)byteArray[0] << 24);
    value |= ((int)byteArray[1] << 16);
    value |= ((int)byteArray[2] << 8);
    value |= ((int)byteArray[3]);

    return value;
}
