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
    (void) printf("started shipping\n"); //remove later as printf is not thread safe




    //gpio_set_direction((gpio_num_t)CherpPin, GPIO_MODE_OUTPUT);






    printf("<<StartStart>>");
    //running loop
    while (1) {


        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second



        //Handle the Outgoing massage Que
        ThreadMessages();

    }
}



void SendQue(QueueHandle_t Queue) {
    MSG received_data_from_queue;
    while(uxQueueMessagesWaiting(Queue)>0){
        if (xQueueReceive(Queue, &received_data_from_queue, portMAX_DELAY)) {
            //int length= strlen((const char *)(data));



            Box board;
            (void) memset(board.data,0,sizeof(board));
            board.delimiter='\a';
            board.VPID=received_data_from_queue.VPID;
            board.Stream='D';
            (void) memset(board.data,0,sizeof(board.data));
            (void) strncpy((char*)board.data,(char*)received_data_from_queue.data,COMS_SIZE-1);
            //(void) uart_write_bytes(UART_NUM, "\a", 1);
            (void) uart_write_bytes(UART_NUM, (const void*)&board, sizeof(Packet));

        }
    }
}

void ThreadMessages() {
    /*if (uxQueueMessagesWaiting(ExchangeQueue) > 0) {
        printf("EXCHANGE\n");
        SendQue(ExchangeQueue);
        printf("\r\n\r\n");
    }

    if (uxQueueMessagesWaiting(RecationQueue) > 0) {
        printf("RECATION\n");
        SendQue(RecationQueue);
        printf("\r\n\r\n");
    }
*/

#ifdef DEBUG
    if (uxQueueMessagesWaiting(DebugQueue) > 0) {
        //printf("DEBUG\n");
        SendQue(DebugQueue);
        //printf("\r\n\r\n");
    }
#endif

}
