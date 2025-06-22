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
    (void) printf("started shipping\n"); //Keep as Printf just in case it goes wrong







    //running loop
    //DoNot add upper Bounds
    while (1) {


        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second



        //Handle the Outgoing massage Que
        ThreadMessages();

    }
}



void SendQue(QueueHandle_t Queue, char type, int VPID) {
    char data[COMS_SIZE] = {0};
    char packet[COMS_SIZE + 2] = {0};  // 1 byte VPID + 1 byte type + data

    while (uxQueueMessagesWaiting(Queue) > 0) {
        if (xQueueReceive(Queue, data, portMAX_DELAY)) {


            //packet[0] = (char)VPID;
            //packet[1] = type;
            //memcpy(&packet[2], data, COMS_SIZE);  // Safely copy message

            // Uncomment to debug contents
            // printf("Sending: VPID=%d, Type=%c, Data[0]=%c\n", VPID, type, data[0]);
            //printf("%d%c%s",VPID,type,data);
            //uart_write_bytes(UART_NUM, packet, sizeof(packet));
        }
    }
}

 void ThreadMessages() {
    //Add VPID later by mixing it in the message ques
     if (uxQueueMessagesWaiting(ExchangeQueue) > 0) {
         SendQue(ExchangeQueue,'E',0);

     }

     if (uxQueueMessagesWaiting(RecationQueue) > 0) {
         SendQue(RecationQueue,'R',0);
     }


 #ifdef DEBUG
     if (uxQueueMessagesWaiting(DebugQueue) > 0) {
         SendQue(DebugQueue,'D',0);
     }
 #endif

}
