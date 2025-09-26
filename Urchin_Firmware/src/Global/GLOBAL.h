//
// Created by gabri on 3/24/2025.
//

#ifndef GLOBAL_H
#define GLOBAL_H




#define CherpPin 2
#define PIDNUM 256
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

#ifdef __cplusplus
}
#endif



#endif //GLOBAL_H
