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
void Shipping(void *pvParameters) {
    //(void) printf("started shipping\n"); //remove later as printf is not thread safe

    ExchangeQueue= xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);
    if (ExchangeQueue==NULL) {return;}

    RecationQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);
    if (RecationQueue==NULL) {return;}

    DebugQueue = xQueueCreate(MSG_QUEUE_LENGTH, MSG_ITEM_SIZE);
    if (DebugQueue==NULL) {return;}


    gpio_set_direction((gpio_num_t)CherpPin, GPIO_MODE_OUTPUT);




    TapeRoll TicketTape;
    RollINIT(&TicketTape);




    //setup massage buffer
    uint8_t data[COMS_SIZE];



    static Context Basic[]={
        {"ReqTicket",9,ReqTicket},
        {"PunchTicket",11,PunchTicket},
        {"CloseTicket",11,CloseTicket},
        {"TicketInfo",10,TicketInfo},
        {"GetHealth",9,GetHealth}
    };


    //Context *CurrentConext[PIDNUM];
    Context CurrentConext=*Basic;



    //running loop
    while (1) {
        //(void) printf("started shipping loop\n");
        (void) PrintfToPI(DebugQueue,"shipping loop");
        (void) PrintfToPI(RecationQueue,"react");
        (void) PrintfToPI(ExchangeQueue,"data");

        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second




        // Read data from UART
        int len = uart_read_bytes(UART_NUM, data, COMS_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len > 0) {

            data[len] = '\0';


            //(void) PrintfToPI(DebugQueue,"ESP-32 Received:%s\n", data);
            //(void) PrintfToPI(DebugQueue,"test");
            //prosses data

            //ProcessRequest(CurrentConext,data);
            //(void) PrintfToPI(DebugQueue,"test2");
        } else {
            // no message is receved so do nothing
        }

        //Handle the Outgoing massage Que
        //printf("que:%d",uxQueueMessagesWaiting(MSGQueue));
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
