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



void SendQue(QueueHandle_t Queue,char type, int VPID) {
    char data[COMS_SIZE];
    char packet[COMS_SIZE+2];
    while(uxQueueMessagesWaiting(Queue)>0){
        if (xQueueReceive(Queue, data, portMAX_DELAY)) {
            char* Curent = (char*)packet;
            Curent[0] = (char)VPID;
            Curent[1] = type;
            Curent=Curent+2;
            strncpy(Curent,(char*)data,COMS_SIZE);
            //
            // SendingFrame packet;
            // memset(&packet, 0, sizeof(packet));
            // strncpy(packet.MSG,(char *)data,COMS_SIZE-1);
            // packet.MSG[COMS_SIZE-1] = '\0';
            // packet.Convo=type;
            // packet.VPID=VPID;


            //printf("%s",data);
            //printf("Sending: VPID=%u, type=%c, first byte of data=%c", packet[0], packet[1], packet[2]);
            //printf((char*)packet);
            (void) uart_write_bytes(UART_NUM, packet, COMS_SIZE+2);

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
