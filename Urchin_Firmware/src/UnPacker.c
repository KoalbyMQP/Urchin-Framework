#include <sys/cdefs.h>
#include "esp_task_wdt.h"
//
// Created by gabri on 3/24/2025.
// !!! do you best you use only C in this file
//

#include "UnPacker.h"

#include <coms.h>
#include "shipping.h"
#include "Errors.h"
#include "TicketNum.h"
#include "MSGQueue.h"
#include "GLOBAL.h"

//#include "Herkulex.h"

#define HX_UART_NUM UART_NUM_2

_Noreturn void receiving(void *pvParameters){

    //HerkulexClass Herkulex;
    //Herkulex.beginSerial3(115200); //open serial port 1
    //Herkulex.reboot(7); //reboot
    //erkulex.initialize();

    RollINIT(&TicketTape);


    //setup massage buffer
    //uint8_t data[COMS_SIZE];

    Box LocalBox;





    static Context Basic[]={
            {"ReqTicket",9,ReqTicket},
            {"PunchTicket",11,PunchTicket},
            {"CloseTicket",11,CloseTicket},
            {"TicketInfo",10,TicketInfo},
            {"GetHealth",9,GetHealth}
    };



    Context *CurrentConext[PIDNUM];
    CurrentConext[0]=Basic;

    //PrintfToPI(DebugQueue,0,"test DebugQueue");
    int count=0;
    while (1){
        memset(&LocalBox,0,sizeof(Box));

        PrintfToPI(DebugQueue,0,"test DebugQueue%d",count);
        count++;
        if (count==32){abort();}
        //PrintfToPI(ExchangeQueue,"test ExchangeQueue");
        //PrintfToPI(RecationQueue,   "test RecationQueue");


        vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay for 0.1 second
        //(void) PrintfToPI(ExchangeQueue,"hello");
        esp_task_wdt_reset();

        char delimiter;
        //uart_read_bytes(UART_NUM,&delimiter,1,100 / portTICK_PERIOD_MS);
        //if (delimiter=='\a') {
            int len=uart_read_bytes(UART_NUM, &LocalBox, sizeof(Box), 100 / portTICK_PERIOD_MS);
                if (len == sizeof(Box)) {
                    //Processes data
                   //PackfToPI(DebugQueue,0,&LocalBox,sizeof(Box));


                }

        //}


    }




}




//Basic statements
//-----------------------------------------------------------------


int ReqTicket(const char* buffer){
(void)PrintfToPI(DebugQueue,0,"\n\nReqTicket called");
    //PrintTape(&TicketTape);
    //Get Ticket

    unsigned int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        (void) PrintfToPI(ExchangeQueue,0,"Code:%d",NoFreeTicket);
        return -1;
    }

    PrintfToPI(DebugQueue,0,"Ticket:%d\n",Ticket);
    //Check out ticket
    checkOut(&TicketTape,Ticket);
    //send OK with ticket
    (void) PrintfToPI(ExchangeQueue,0,"Code:%d \nTicket%d ",OK,Ticket);
    (void) PrintfToPI(ExchangeQueue,0,"Tape%u",TicketTape);

  return 0;
}







int PunchTicket(const char* buffer) {

    /*
     PrintfToPI(DebugQueue,"Set Led Green");

    Herkulex.setLed(7, LED_GREEN2_HRAMWRITE); //set the led to green
    PrintfToPI(DebugQueue,"Status:");
    PrintfToPI(DebugQueue,"",Herkulex.stat(n)); //verify error code
    Herkulex.end();
    */

    (void) PrintfToPI(ExchangeQueue,0,"PunchTicket not added");
    return 0;
}


int CloseTicket(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,0,"CloseTicket not added");
return 0;
}



int TicketInfo(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,0,"TicketInfo not added");
return 0;
}



int GetHealth(const char* buffer) {
    if (0== strcmp("FreeRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,0,"%d",xPortGetFreeHeapSize());

    }
    if (0== strcmp("TotalRam",buffer)) {
        (void) PrintfToPI(ExchangeQueue,0,"%d",xPortGetMinimumEverFreeHeapSize());
    }

    if (0== strcmp("CPU",buffer)) {

    }
    if (0== strcmp("AckCheck",buffer)) {

    }


    //(void) PrintfToPI(ExchangeQueue,"GetHealth is being added");
    //DoSomething();
return 0;
}

//-----------------------------------------------------------------------



//Helppers
//------------------------------------------------------------------------

int ProcessRequest(Context Commands[],const uint8_t buffer[]) {
        int i=0;
        int found=0;
        int error=0;

    (void) PrintfToPI(DebugQueue,0,"ProcessRequest:%s",buffer);
        while (i < NumOfActions && !found){
            if (0==strncmp((char*)buffer,Commands[i].Name,Commands[i].depth)) {
                error=Commands[i].function(SkipFoward((char*)buffer,Commands[i].depth));
                found=1;
            }
            i++;
            }
    if (!found) {return CommandNotFound;}
    return error;

}


const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}

