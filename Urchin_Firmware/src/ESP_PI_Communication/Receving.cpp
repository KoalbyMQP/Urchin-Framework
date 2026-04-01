//
// Created by gabri on 7/13/2025.
//

#include "Receving.h"
#include "Conversation/UnPacker.h"
#include "Ticketing//TicketNum.h"
#include "Coms.h"
#include "Errors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "Herkulex/Herkulex.h"


#define HX_UART_NUM UART_NUM_2

[[noreturn]] void receiving(void *pvParameters){



    RollINIT(&TicketTape);

    TicketTapeMutex = xSemaphoreCreateMutex();


    //setup massage buffer
    //uint8_t data[COMS_SIZE];


    (void)UnpackerInit();



//setting up conversation
    static Context Basic[]={//has max size see Receving.h
            /*{"ReqTicket",9,ReqTicket,NULL,0},*/
            {"FormatTicket",12,FormatTicket,NULL,0},
            {"LoadTicket",10,LoadTicket,NULL,0},
            {"PunchTicket",11,PunchTicket,NULL,0},
            {"CloseTicket",11,CloseTicket,NULL,0},
            {"TicketInfo",10,TicketInfo,NULL,0},
            {"GetHealth",9,GetHealth,NULL,0},
            {"Bridge",6,Bridge,NULL,0}
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



    while (true) {
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

                        ProcessRequest(LocalBox->VPID,CurrentConext[LocalBox->VPID],(uint8_t*) LocalBox->data);

                        box_pos = 0;
                        syncing = true; // look for next frame
                    }
                }
            }
        }

        esp_task_wdt_reset();
        //taskYIELD();
    }


}

//Helppers
//------------------------------------------------------------------------



int ProcessRequest(unsigned char VPID ,Context Commands[],const uint8_t buffer[]) {
    int i=0;
    int found=0;
    int error=0;

    //(void) PrintfToPI(DebugQueue,0,"ProcessRequest:%s",buffer);
    while (i < NumOfActions && !found){
        if (0==strncmp((char*)buffer,Commands[i].Name,Commands[i].depth)) {
            error=Commands[i].function(VPID,SkipFoward((char*)buffer,Commands[i].depth));
            found=1;
        }
        i++;
    }
    if (!found) {return URCHIN_ERROR_CommandNotFound;}
    return error;

}


const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}
