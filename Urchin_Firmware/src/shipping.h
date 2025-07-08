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




#pragma pack(push, 1) // Ensure no padding bytes

typedef struct {
    char delimiter;
    uint8_t VPID;
    uint8_t Stream;
    char data[COMS_SIZE] ;
    } Box;

#pragma pack(pop)



/**
 * @brief Shipping task process, Ment to handle outgoing communication to the PI
 * @details BEHAVIOR:Will Send all outgoing message streams with PID header and loop.
 * @param pvParameters leave empty
 * @date 2025-03-24
 * @author Gabriel Weaver
 */
_Noreturn void Shipping(void *pvParameters);

/**
 * 
 * @param Queue
 * @date 7/4/2025
 * @author Gabriel Weaver
 */
void SendQue(QueueHandle_t Queue, char Stream);


/**
 *
 */
void ThreadMessages();

/**
 *
 * @param VPID
 * @param Stream
 * @param buff
 * @param size
 * @return
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
