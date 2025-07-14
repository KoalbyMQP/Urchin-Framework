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
 * @brief Shipping task process, Ment to handle outgoing communication to the PI
 * @details BEHAVIOR:Will Send all outgoing message streams with PID header and loop.
 * @param pvParameters leave empty
 * @date 2025-03-24
 * @author Gabriel Weaver
 */
_Noreturn void Shipping(void *pvParameters);


/**
 * Send the whole queue over the USB
 * @param Queue the queue to be sent
 * @param Stream type of message
 * @date 7/4/2025
 * @author Gabriel Weaver
 */
void SendQue(QueueHandle_t Queue, char Stream);


/**
 *
 */
void ThreadMessages();

/**
 * Sends a single message over uart USB
 * @param VPID Virtual Process ID (Used to separate different API instances)
 * @param Stream Type of message
 * @param buff Message
 * @param size length of message, MAX=1024

 * @retval 0 Success
 * @retval -1 Failed
 */
int SendMessage(const uint8_t VPID, const char Stream, const uint8_t buff[], const size_t size);



#ifdef __cplusplus
}
#endif



#define MSG_QUEUE_LENGTH 16

typedef struct{
    uint8_t VPID;
    char data[COMS_SIZE];
}MSG;

#define MSG_ITEM_SIZE (sizeof(MSG))

#define MissBeatPin 25
#endif //SHIPPING_H
