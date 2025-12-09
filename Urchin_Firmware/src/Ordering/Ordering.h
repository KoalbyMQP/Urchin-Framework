//
// Created by gabri on 11/6/25.
//

#ifndef URCHIN_FIRMWARE_ORDERING_H
#define URCHIN_FIRMWARE_ORDERING_H
#include "MSGQueue.h"
#define ActionQueDepth 5


void ActionQueINIT();


extern QueueHandle_t InterruptQueue;
extern QueueHandle_t AsynchronousQueue;
extern QueueHandle_t SequentialQueue;

#endif //URCHIN_FIRMWARE_ORDERING_H


