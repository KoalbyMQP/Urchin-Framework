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
    uint8_t VPID;
    uint8_t Stream;
    char data[1024] ;
    } Box;

#pragma pack(pop)



/**
 * @brief Shipping task process, Ment to handle out going communication to the PI
 * @details BEHAVOR:Will Send all outgoing message streams with PID header and loop.
 * @param pvParameters leave empty
 * @date 2025-03-24
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



#define MSG_QUEUE_LENGTH 16
#define MSG_ITEM_SIZE (COMS_SIZE*sizeof(char))
#define MissBeatPin 25
#endif //SHIPPING_H
