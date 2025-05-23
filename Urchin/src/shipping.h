//
// Created by gabri on 3/23/2025.
//

#ifndef SHIPPING_H
#define SHIPPING_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "coms.h"

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"

extern uart_config_t uart_config;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shipping task process, Ment to handle all communcation to the PI
 * @details BEHAVOR:\n Shipping will handel the Heart Beat.\n The loop will read one message then send all outgoing message then loop.
 * @param pvParameters leave empty
 * @date 2025-03024
 * @author Gabriel Weaver
 */
void Shipping(void *pvParameters);

/**
 * @brief Called when the PI misses the Heart Beat
 * @date 2025-03024
 * @author Gabriel Weaver
 */
void MissHeartbeat();

#ifdef __cplusplus
}
#endif



#define MSG_QUEUE_LENGTH 30
#define MSG_ITEM_SIZE (1024*sizeof(char))
#define MissBeatPin 25
#endif //SHIPPING_H
