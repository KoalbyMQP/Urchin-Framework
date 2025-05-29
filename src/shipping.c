#include <sys/cdefs.h>
//
// Created by gabri on 3/23/2025.
//
#include "GLOBAL.h"
#include "shipping.h"
#include "Ticket.h"
#include "TicketNum.h"
#include "UnPacker.h"
#include "MSGQueue.h"
QueueHandle_t ExchangeQueue;
QueueHandle_t RecationQueue;
QueueHandle_t DebugQueue;
// cppcheck-suppress unusedFunction
_Noreturn void Shipping(void *pvParameters) {
    //(void) printf("started shipping\n"); //remove later as printf is not thread safe

    ExchangeQueue= xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    RecationQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);

    DebugQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);


    //gpio_set_direction((gpio_num_t)CherpPin, GPIO_MODE_OUTPUT);







    //running loop
    while (1) {


        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second



        //Handle the Outgoing massage Que
        ThreadMessages();

    }
}



void SendQue(QueueHandle_t Queue) {
    uint8_t data[COMS_SIZE];
    while(uxQueueMessagesWaiting(Queue)>0){
        if (xQueueReceive(Queue, &data, portMAX_DELAY)) {
            int length= strlen((const char *)(data));
            (void) uart_write_bytes(UART_NUM, data, length);
            (void) printf("\n\n");
        }
    }
}

void ThreadMessages() {
    if (uxQueueMessagesWaiting(ExchangeQueue) > 0) {
        printf("EXCHANGE\n");
        SendQue(ExchangeQueue);
        printf("\r\n\r\n");
    }

    if (uxQueueMessagesWaiting(RecationQueue) > 0) {
        printf("RECATION\n");
        SendQue(RecationQueue);
        printf("\r\n\r\n");
    }


#ifdef DEBUG
    if (uxQueueMessagesWaiting(DebugQueue) > 0) {
        printf("DEBUG\n");
        SendQue(DebugQueue);
        printf("\r\n\r\n");
    }
#endif

}
