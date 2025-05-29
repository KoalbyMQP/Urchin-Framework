//
// Created by gabri on 3/24/2025.
//

#include "UnPacker.h"

#include <coms.h>
#include "shipping.h"
#include "Errors.h"
#include "TicketNum.h"
#include "MSGQueue.h"
#include "GLOBAL.h"




//Basic statements
//-----------------------------------------------------------------

/**
 * To be called when the PI is asking for a ticket
 * @param buffer buffer after the "ReqTicket" command (Not Used)
 * @return An error code
 */
int ReqTicket(const char* buffer){


    //Get Ticket
    int Ticket = FindFree(&TicketTape);

    //send NoFreeTicket if non are found
    if (Ticket == -1) {
        (void) PrintfToPI(ExchangeQueue,"Code:%d",NoFreeTicket);
        return -1;
    }

    //Check out ticket
    checkOut(&TicketTape,Ticket);

    //send OK with ticket
    (void) PrintfToPI(ExchangeQueue,"Code:%d \n%d ",OK,Ticket);
  return 0;
}


/**
 * To be called when the PI is asking for a ticket to be punched(sending a command)
 * @param buffer the constance of the punch
 * @return An error
 */
int PunchTicket(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,"PunchTicket not added");
    return 0;
}

/**
 * To be called when the Pi wants to close a ticket early
 * @param buffer buffer after the "ReqTicket" command (Not Used)
 * @return An error
 */
int CloseTicket(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,"CloseTicket not added");
return 0;
}


/**
 * To be called when the Pi wants to know the status of a Ticket
 * @param buffer Ticket number
 * @return Error code
 */
int TicketInfo(const char* buffer) {

    (void) PrintfToPI(ExchangeQueue,"TicketInfo not added");
return 0;
}


/**
 * To be called when the Pi wants a health update from the esp32
 * @param buffer the Type of health
 * @return An Error
 */
int GetHealth(const char* buffer) {
    (void) PrintfToPI(ExchangeQueue,"GetHealth is being added");
    DoSomething();
return 0;
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
int ProcessRequest(Context Commands[],const uint8_t buffer[]) {
        int i=0;
        int found=0;
        int error=0;

    (void) PrintfToPI(DebugQueue,"ProcessRequest:%s",buffer);
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

/**
 * Skips the reader forward by the distace
 * @param buffer Char buffer
 * @param distance Distance to truncate
 * @return New begging of string
 */
const char* SkipFoward(const char buffer[],unsigned int distance) {
    return &buffer[distance];
}

