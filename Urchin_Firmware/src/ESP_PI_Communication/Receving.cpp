//
// Created by gabri on 7/13/2025.
//

#include "Receving.h"
#include "Conversation/UnPacker.h"
#include "Ticketing//TicketNum.h"
#include "Coms.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "Herkulex/Herkulex.h"


#define HX_UART_NUM UART_NUM_2

void receiving(void *pvParameters){

    HerkulexClass Herkulex;
    Herkulex.begin(UART_NUM_1,115200,16,17); //115200 is default for DRS-0601
    //Herkulex.reboot(7); //reboot
    //Herkulex.initialize();
    Herkulex.reboot(7);
    Herkulex.torqueON(7);

    Herkulex.setLed(7, LED_GREEN_HRAMWRITE);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    Herkulex.setLed(7, LED_BLUE_HRAMWRITE);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    Herkulex.setLed(7, LED_GREEN2_HRAMWRITE);

    RollINIT(&TicketTape);

    TicketTapeMutex = xSemaphoreCreateMutex();


    //setup massage buffer
    //uint8_t data[COMS_SIZE];






//setting up conversation
    static Context Basic[]={//has max size see unpacker.h
            {"ReqTicket",9,ReqTicket,NULL,0},
            {"FormatTicket",12,FormatTicket,NULL,0},
            {"LoadTicket",10,LoadTicket,NULL,0},
            {"PunchTicket",11,PunchTicket,NULL,0},
            {"CloseTicket",11,CloseTicket,NULL,0},
            {"TicketInfo",10,TicketInfo,NULL,0},
            {"GetHealth",9,GetHealth,NULL,0}
    };



    Context *CurrentConext[PIDNUM];
    CurrentConext[0]=Basic;

    //receiving buffer to hold partial messages - 256 as works well with seral
    uint8_t rx_buffer[256];

    //receiving buffer to hold developing messages
    uint8_t box_buffer[sizeof(Box)];

    //offset of box
    int box_pos = 0;

    //if we are looking for the delimiter
    bool syncing = true;


    bool working = true;
    while (working) {
        int len = uart_read_bytes(UART_NUM, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(20));
        if (len > 0) { //Check if there was something to receive

            for (int i = 0; i < len; i++) { //Check each byte in message for delimiter
                uint8_t byte = rx_buffer[i];


                // Look for delimiter then aligns the box
                if (syncing) {
                    if (byte == '\a') {
                        box_buffer[0] = byte;
                        box_pos = 1;
                        syncing = false;
                    }

                } else {
                    box_buffer[box_pos++] = byte;

                    if (box_pos == sizeof(Box)) {
                        // Full frame received
                        Box *LocalBox = (Box *)box_buffer;

                        // Process the Box here

                        ProcessRequest(CurrentConext[LocalBox->VPID],(uint8_t*) LocalBox->data);

                        box_pos = 0;
                        syncing = true; // look for next frame
                    }
                }
            }
        }

        esp_task_wdt_reset();
    }


}