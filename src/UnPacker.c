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

void InitProcessRequestBasic(Context * Basic[]) {
    Basic[0]=&Context{"ReqTicket",9,ReqTicket};
    Basic[1]=&Context{"PunchTicket",11,PunchTicket};
    Basic[2]=&Context{"CloseTicket",11,CloseTicket};
    Basic[3]=&Context{"TicketInfo",10,TicketInfo};
    Basic[4]=&Context{"GetHealth",9,GetHealth};

}



//Basic statements
//-----------------------------------------------------------------

/**
 * To be called when the PI is asking for a ticket
 * @param buffer buffer after the "ReqTicket" command (Not Used)
 * @return An error code
 */
int ReqTicket(const char* buffer){
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


/**
 * To be called when the PI is asking for a ticket to be punched(sending a command)
 * @param buffer the constance of the punch
 * @return An error
 */
int PunchTicket(const char* buffer) {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"PunchTicket not added");
}

/**
 * To be called when the Pi wants to close a ticket early
 * @param buffer buffer after the "ReqTicket" command (Not Used)
 * @return An error
 */
int CloseTicket(const char* buffer) {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"CloseTicket not added");

}


/**
 * To be called when the Pi wants to know the status of a Ticket
 * @param buffer Ticket number
 * @return Error code
 */
int TicketInfo(const char* buffer) {
    extern QueueHandle_t MSGQueue;
    (void) PrintfToPI(MSGQueue,"TicketInfo not added");
}


/**
 * To be called when the Pi wants a health update from the esp32
 * @param buffer the Type of health
 * @return An Error
 */
int GetHealth(const char* buffer) {

}

//-----------------------------------------------------------------------



//Helppers
//------------------------------------------------------------------------
/**
 * This goes through a set of Commands calling its fucntion on the buffer
 * @param Commands Array of Contexts
 * @param buffer The char buffer that was receved
 * @return An arror code
 */
int ProcessRequest(Context Commands[],const char buffer[]) {
        int i=0;
        int found=0;
        int error=0;
        while (i < NumOfActions && !found){
            if (0==strncmp(buffer,Commands[i].Name,Commands[i].depth)) {
                error=Commands[i].function(SkipFoward(buffer,Commands[i].depth));
                found=1;
            }
            i++;
            }
    if (!found) {return CommandNotFound;}
    return error;




}

/**
 * Skips the reader forward by the distace
 * @param buffer Char buffer
 * @param distance Distance to truncate
 * @return New begging of string
 */
const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}

