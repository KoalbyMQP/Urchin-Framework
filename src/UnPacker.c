//
// Created by gabri on 3/24/2025.
//

#include "UnPacker.h"

#include <coms.h>

#include "Errors.h"
#include "Ticket.h"

Context * Basic[NumOfActions];
Context *Load[NumOfActions];
Context *Health[NumOfActions];
Context *Get[NumOfActions];
Context *Sleep[NumOfActions];

void InitProcessRequest() {

    Basic[0]=&Context{"ReqTicket",9,ReqTicket};

    // "load","Health","Get", "DisableSleep" , "EnableSleep"

}


int ReqTicket(){
    extern TapeRoll TicketTape;
    extern QueueHandle_t MSGQueue;

    //Get Ticket
    int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        (void) PrintfToPI(MSGQueue,"Code:%d",NoFreeTicket);
        return -1;
    }

    //Check out ticket
    checkOut(&TicketTape,Ticket);

    //send OK with ticket
    (void) PrintfToPI(MSGQueue,"Code:%d \n%d ",OK,Ticket);
  return 0;
}

int PunchTicket() {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"PunchTicket not added");
}

int CloseTicket() {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"CloseTicket not added");

}

int TicketInfo() {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"TicketInfo not added");
}

int GetHealth() {

}



int ProcessRequest(const char buffer[]) {
        int i=0;
        int found=0;
        int error=0;
        while (i < NumOfActions && !found){
            if (0==strncmp(buffer,Basic[i]->Name,Basic[i]->depth)) {
                error=Basic[i]->function(SkipFoward(buffer,Basic[i]->depth));
                found=1;
            }
            i++;
            }
    return error;




}





int LED_Action() {
return 0;
}

const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}

