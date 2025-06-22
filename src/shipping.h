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



    typedef struct __attribute__((packed))  {
        uint8_t VPID;
        char Convo;
        char MSG[1024];
    }SendingFrame;

/**
 * @brief Shipping task process, Ment to handle out going communication to the PI
 * @details BEHAVOR:Will Send all outgoing message streams with PID header and loop.
 * @param pvParameters leave empty
 * @date 2025-03-24
 * @author Gabriel Weaver
 */
_Noreturn void Shipping(void *pvParameters);

/**
 * @brief Sends one Queue out with the VPID and Type
 * @details
 * @date 6/22/2025
 * @author Gabriel Weaver
 * @param Queue The Message Queue
 * @param type Type of message
 * @param VPID Virtual Procsses ID, Maps back to an ID in the PI process Multiplexor
 */
    void SendQue(QueueHandle_t Queue,char type, int VPID);


/**
 * @brief Empties the queues by sendinf them out in shipping
 * @details
 * @date 6/22/2025
 * @author Gabriel Weaver
 */
void ThreadMessages();



#ifdef __cplusplus
}
#endif



#define MSG_QUEUE_LENGTH 16
#define MSG_ITEM_SIZE (COMS_SIZE*sizeof(char))
#endif //SHIPPING_H
