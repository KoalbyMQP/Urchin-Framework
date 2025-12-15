//
// Created by gabri on 11/6/25.
//

#include "Ordering.h"

 QueueHandle_t InterruptQueue;
 QueueHandle_t AsynchronousQueue;
 QueueHandle_t SequentialQueue;


void ActionQueINIT() {
    AsynchronousQueue= xQueueCreate(ActionQueDepth, sizeof(Ticket));
    SequentialQueue = xQueueCreate(ActionQueDepth, sizeof(Ticket));
    InterruptQueue = xQueueCreate(1, sizeof(Ticket));
}


int PunchToQue(Ticket *ticket) {
    QueueHandle_t Target = NULL;

    switch (ticket->type) {
        case 'A':
            Target=AsynchronousQueue;
            break;
        case 'S':
            Target=SequentialQueue;
            break;
        case 'R':
            Target=SequentialQueue;
            break;
        case 'I':
            Target=InterruptQueue;
            break;
        default:
            return -2;
    }


    if(pdPASS != xQueueSend(Target,ticket,portMAX_DELAY)){
        return -1;
    }
    return 0;
}
