//
// Created by gabri on 3/24/2025.
//

#ifndef GLOBAL_H
#define GLOBAL_H



#define LedPin GPIO_NUM_2
#define CherpPin GPIO_NUM_5
#define SlapPin GPIO_NUM_4




#define PIDNUM 256
#include <soc/gpio_num.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @param value
 * @param byteArray
 */
void unsignedIntToBytes(unsigned int value, unsigned char* byteArray);


/**
 *
 * @param byteArray
 * @return
 */
unsigned int BytesToUnsignedInt(const unsigned char* byteArray);


/**
 *
 * @param byteArray
 * @return
 */
int BytesToInt(const unsigned char* byteArray);

    void Pulse(gpio_num_t Pin);
    void Slap();
    void LED();
    void Chirp();

typedef union {
    unsigned int UI;
    int I;
    char String[4];
    unsigned char UString[4];
    float F;
}WebConversion;


#ifdef __cplusplus
}
#endif



#endif //GLOBAL_H
