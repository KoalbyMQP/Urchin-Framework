/**
 * @file Coms.h
 * @brief User call for Communication
 * @author Gabriel Weaver
 * @date 3-23-2025
 * @version 1.0
 * @defgroup coms Coms
 */

#ifndef COMS_H
#define COMS_H

#define COMS_SIZE 1024

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include "freertos/queue.h"
// #include "Annotations.h"

#define UART_NUM UART_NUM_0  // Use UART0 (default for USB serial)



//Used to remove padding so python can take it apart
#pragma pack(push, 1) // Ensure no padding bytes

/**
 *Used to send messages to and from PI
 * @struct Box
 */
typedef struct {
   /**
    * Char to denote start of box should be '\a'
    */
    char delimiter;

    /**
     * Virtual Process ID (Used to separate different API instances)
     */
    uint8_t VPID;

    /**
     * Type of message
     */
    uint8_t Stream;

    /**
     * Message data
     */
    char data[COMS_SIZE] ;

} Box;

#pragma pack(pop)




//Return codes
#define Success 0
#define QueFull (-1)
//


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Adds the string to the queue
 * @details This is a thread safe comuncation system to communcate to the machine over the usb interface
 * @note This fucntion adds a message to a queue that is handled by the Shipping thread. Thus Shipping thread must be running
 *
 * @param MsgQueue[out] The outgoing message queue
 * @param buff[in] the char buffer
 * @param buff_size[in] the size of the buffer
 *
 * @return The error state of the fucntion
 * @retval 0 Success
 * @retval -1 queue full
 * @retval >0 number of bits over the max message size
 * \n \n Example Usage:
 * @code
 * int Error=PackfToPI(MsgQueue, "Hello World", 12);
 * if (error==QueFull){printf("Que full error");}
 * if (error>0){printf("Buffer to large");}
 * @endcode
 * @code
 * (void) PackfToPI(MsgQueue, "Hello World", 12);
 * @endcode
 * @see COMS_SIZE
 * @author Gabriel Weaver
 * @date 2025-03-24
 * @version 1.0
 */
 int PackfToPI(QueueHandle_t MsgQueue, const uint8_t VPID, const char buff[], size_t buff_size);

/**
 * @brief Prints to the Pi over the USB bridge
 * @details This is a thread safe comuncation system to communcate to the machine over the usb interface
 * @note This fucntion adds a message to a queue that is handled by the Shipping thread. Thus Shipping thread must be running
 *
 * @param MsgQueue[out] The outgoing message Que
 * @param VPID virtual process id
 * @param format[in] A string that may contain format specifiers like %d, %s, etc., which control the formatting of subsequent arguments.
 * @param ...[in] A variable number of arguments to be formatted and printed according to the format string.
 *
 * @return The error state of the fucntion
 * @retval 0 Success
 * @retval -1 queue full
 * @retval >0 number of bits over the max message size
 * \n \n Example Usage:
 * @code
 * int Error=PrintfToPI(MsgQueue, "Name:%s, Year:%d, state:%c","bob",2034,'J');
 * if (error==QueFull){printf("Que full error");}
 * if (error>0){printf("Buffer to large");}
 * @endcode
 * @code
 * (void) PrintfToPI(MsgQueue, "Name:%s, Year:%d, state:%c","bob",2034,'J');
 * @endcode
 * @see PackToPI
 * @author Gabriel Weaver
 * @date 2025-03-24
 * @version 1.0
 */
 int PrintfToPI(QueueHandle_t MsgQueue, const uint8_t VPID, const char *format, ...);

#ifdef __cplusplus
}
#endif


#endif //COMS_H
