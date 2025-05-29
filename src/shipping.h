#include <sys/cdefs.h>
//
// Created by gabri on 3/23/2025.
//

#ifndef SHIPPING_H
#define SHIPPING_H




#include "coms.h"

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"

extern uart_config_t uart_config;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Shipping task process, Ment to handle out going communication to the PI
 * @details BEHAVOR:Will Send all outgoing message streams with PID header and loop.
 * @param pvParameters leave empty
 * @date 2025-03024
 * @author Gabriel Weaver
 */
_Noreturn void Shipping(void *pvParameters);

/**
 * 
 * @param Queue 
 */
void SendQue(QueueHandle_t Queue);




    void ThreadMessages();



#ifdef __cplusplus
}
#endif



#define MSG_QUEUE_LENGTH 30
#define MSG_ITEM_SIZE (1024*sizeof(char))
#define MissBeatPin 25
#endif //SHIPPING_H
