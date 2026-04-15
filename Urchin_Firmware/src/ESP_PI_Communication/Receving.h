//
// Created by gabri on 7/13/2025.
//

#ifndef URCHIN_FIRMWARE_RECEVING_H
#define URCHIN_FIRMWARE_RECEVING_H

#include "Global/GLOBAL.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"



#ifdef __cplusplus
extern "C" {
#endif


#define ContextWordSize 15
#define NumOfActions 8


    typedef struct Context Context;  // Forward declaration

    /**
     *@brief A context is a function to be called when a word is seen in message from the PI
     */
    typedef struct Context{
        /**
         * The word or phrase that will signal a kind of action
         */
        const char Name[ContextWordSize];

        /**
         * The length of Name
         */
        const unsigned short int depth;

        /**
         * The function to be called when the name was mentioned
         * @param VPID The VPID this function was called from
         * @param buffer[] The remaining string after the "Name"
         */
        int (*function)(unsigned char VPID ,const char buffer[]);

        /**
         * list contexts to branch to
         */
        Context* branch;

        /**
         * Number of contexts in list
         */
        const unsigned int size;

    }Context;

/**
     * @brief Receiving task process, Ment to handle incoming going communication to the PI, command validation, and pushing to the Queue
     * @details BEHAVOR:Will receve a message then sent it to the context of its PID then loop.
     * @param pvParameters Empty
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
 [[noreturn]] void receiving(void *pvParameters);


    /**
   * Skips the reader forward by the distace
   * @param buffer Char buffer
   * @param distance Distance to truncate
   * @return New begging of string
   */
    const char* SkipFoward(const char buffer[],unsigned int distance);

    /**
    * This goes through a set of Commands calling its fucntion on the buffer
    * @param Commands Array of Contexts
    * @param buffer The char buffer that was receved
    * @return An arror code
    */
    int ProcessRequest(unsigned char VPID,Context Commands[],const uint8_t buffer[]);

#ifdef __cplusplus
}
#endif


#endif //URCHIN_FIRMWARE_RECEVING_H
