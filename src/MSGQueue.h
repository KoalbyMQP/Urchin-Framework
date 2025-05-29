//
// Created by gabri on 5/25/2025.
//

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t ExchangeQueue;
extern QueueHandle_t RecationQueue;
extern QueueHandle_t DebugQueue;


#ifdef __cplusplus
}
#endif



#endif //MSGQUEUE_H
