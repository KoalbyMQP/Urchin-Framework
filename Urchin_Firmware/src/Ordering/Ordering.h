//
// Created by gabri on 11/6/25.
//

#ifndef URCHIN_FIRMWARE_ORDERING_H
#define URCHIN_FIRMWARE_ORDERING_H


#ifdef __cplusplus
extern "C" {
#endif

#include "MSGQueue.h"
#define ActionQueDepth 5
#include "Ticketing/Ticket.h"

void ActionQueINIT();
int PunchToQue(Ticket *ticket);

extern QueueHandle_t InterruptQueue;
extern QueueHandle_t AsynchronousQueue;
extern QueueHandle_t SequentialQueue;

#ifdef __cplusplus
}
#endif

#endif //URCHIN_FIRMWARE_ORDERING_H
