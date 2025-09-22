#include <sys/cdefs.h>
#include "esp_task_wdt.h"
#include "Ticketing/Ticket.h"
//
// Created by gabri on 3/24/2025.
// !!! do you best you use only C in this file
//





#include "UnPacker.h"

#include <ESP_PI_Communication/Coms.h>
#include "ESP_PI_Communication/Shipping.h"
#include "Global/Errors.h"
#include "Ticketing/TicketNum.h"
#include "ESP_PI_Communication/MSGQueue.h"



#include "Herkulex/Herkulex.h"

Ticket* Tickets[MaxTickets];

//Basic statements
//-----------------------------------------------------------------


int ReqTicket(const char* buffer){
(void)PrintfToPI(DebugQueue,0,"ReqTicket called");
    //PrintTape(&TicketTape);
    //Get Ticket

    unsigned int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        (void) PrintfToPI(ExchangeQueue,0,"Code:%d",URCHIN_ERROR_NoFreeTicket);
        return -1;
    }

    PrintfToPI(DebugQueue,0,"Ticket:%d\n",Ticket);
    //Check out ticket
    checkOut(&TicketTape,Ticket);
    //send OK with ticket
    (void) PrintfToPI(ExchangeQueue,0,"Code:%d \nTicket%d ",URCHIN_OK,Ticket);
    (void) PrintfToPI(ExchangeQueue,0,"Tape%u",TicketTape);

  return 0;
}

int FormatTicket(const char* buffer) {
    const char *TicketNumPoint = buffer;
    const char TicketType = *(buffer+sizeof(int));


    int TicketNum = StrToInt(TicketNumPoint);

    void *alloc = pvPortMalloc(sizeof(Ticket));
    if (alloc == nullptr) {
        //todo throw error
        return -1;
    }else {
        Ticket* TicketPoint= static_cast<Ticket *>(alloc);
        TicketPoint->TicketNum = TicketNum;
        TicketPoint->type = TicketType;
        Tickets[TicketNum] = TicketPoint;
    }
    return 0;
}



int LoadTicket(const char* buffer) {
    (void) PrintfToPI(ExchangeQueue,0,"LoadTicket called");
    return 0;
}




int PunchTicket(const char* buffer) {
    (void) PrintfToPI(ExchangeQueue,0,"PunchTicket called");







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


int StrToInt(const char* buffer) {
    int temp=0;
    temp |= buffer[0] <<24;
    temp |= buffer[1] <<16;
    temp |= buffer[2] <<8;
    temp |= buffer[3];
    return temp;
}


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
    if (!found) {return URCHIN_ERROR_CommandNotFound;}
    return error;

}


const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}
