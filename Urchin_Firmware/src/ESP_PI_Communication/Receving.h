//
// Created by gabri on 7/13/2025.
//

#ifndef URCHIN_FIRMWARE_RECEVING_H
#define URCHIN_FIRMWARE_RECEVING_H

#include "Global/GLOBAL.h"
#include "Adaptability/Binder.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"



#ifdef __cplusplus
extern "C" {
#endif




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
