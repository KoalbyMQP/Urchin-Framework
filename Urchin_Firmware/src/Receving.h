//
// Created by gabri on 7/13/2025.
//

#ifndef URCHIN_FIRMWARE_RECEVING_H
#define URCHIN_FIRMWARE_RECEVING_H

#include "GLOBAL.h"
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
_Noreturn void receiving(void *pvParameters);


#ifdef __cplusplus
}
#endif


#endif //URCHIN_FIRMWARE_RECEVING_H
