//
// Created by gabri on 3/23/2025.
//

#include "shipping.h"
#include "Ticket.h"
#include "UnPacker.h"
// cppcheck-suppress unusedFunction
void Shipping(void *pvParameters) {
    //(void) printf("\n\n\nstarted shipping\n"); //remove later as printf is not thread safe

    QueueHandle_t MSGQueue = (QueueHandle_t) pvParameters;
    if (MSGQueue==NULL) {return;}

    TapeRoll TicketTape;
    RollINIT(&TicketTape);

    //Setup inits
    int side = false;
    int born = false;
    TickType_t heard =xTaskGetTickCount();
    (void) gpio_set_direction((gpio_num_t)(MissBeatPin), GPIO_MODE_OUTPUT);
    (void) gpio_set_level((gpio_num_t)(MissBeatPin), 1);


    //setup massage buffer
    uint8_t data[COMS_SIZE];



    Context * Basic[NumOfActions];
    InitProcessRequestBasic(Basic);



    //running loop
    while (1) {
        //(void) printf("\n\n\nstarted shipping loop\n"); //remove later as printf is not thread safe
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second




        // Read data from UART
        int len = uart_read_bytes(UART_NUM, data, COMS_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len > 0) {

            data[len] = '\0';


            (void) PrintfToPI(MSGQueue,"ESP-32 Received: %s\n", data);
            //prosses data
            ProcessRequest(Basic,data);
        } else {
            // no message is receved so do nothing
        }

        //Handle the Outgoing massage Que
        //printf("que:%d",uxQueueMessagesWaiting(MSGQueue));
        while(uxQueueMessagesWaiting(MSGQueue)>0){
            if (xQueueReceive(MSGQueue, &data, portMAX_DELAY)) {
                int length= strlen((const char *)(data));
                (void) uart_write_bytes(UART_NUM, data, length);
            }
        }

    }
}