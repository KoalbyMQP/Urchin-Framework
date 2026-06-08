//
// Created by gabri on 3/24/2025.

#ifndef UNPACKER_H
#define UNPACKER_H

#include <string.h>
#include "Global/GLOBAL.h"
#include "ESP_PI_Communication/Receving.h"
#include "ESP_PI_Communication/Reaction.h"

#define MAX_VALUES_LEN 20








int UnpackerInit();










    //Helpers
    //-------------------------------------------------

    /**
     *
     * @param buffer
     * @return
     */
    int StrToInt(const char* buffer);


    //--------------------------------------------------------------------



    //Context functions
    //-----------------------------------

    /**
    * To be called when the PI is looking for teh board
    * @param buffer buffer after the "Validate" command (Not Used)
    * @return An error code
    */
    int Validate(unsigned char VPID, const char* buffer);


    /**
    * To be called when the PI is asking for a ticket
    * @param buffer buffer after the "ReqTicket" command (Not Used)
    * @return An error code
    */
    int ReqTicket(unsigned char VPID, const char* buffer);

    /**
    * To be called when the PI is asking for a ticket to be punched(sending a command)
    * @param buffer the constance of the punch
    * @return An error
    */
    int PunchTicket(unsigned char VPID, const char* buffer);

    /**
    * To be called when the Pi wants to close a ticket early
    * @param buffer buffer after the "ReqTicket" command (Not Used)
    * @return An error
    */
    int CloseTicket(unsigned char VPID, const char* buffer);

    /**
    * To be called when the Pi wants to know the status of a Ticket
    * @param buffer Ticket number
    * @return Error code
    */
    int TicketInfo(unsigned char VPID, const char* buffer);

    /**
    * To be called when the Pi wants a health update from the esp32
    * @param buffer the Type of health
    * @return An Error
    */
    int GetHealth(unsigned char VPID, const char* buffer);


    /**
    * To be called when the Pi wants to add a motor entry from the bridge file
    * @param buffer the Type of health
    * @return An Error
    */
    int Bridge(unsigned char VPID, const char* buffer);


    /**
    * To be called when the Pi wants to Load a ticket
    * @param buffer Data for loading
    * @return An Error
    */
    int LoadTicket(unsigned char VPID, const char* buffer);


    /**
    * To be called when the Pi wants to format a new ticket
    * @param buffer format information
    * @return An Error
    */
    int FormatTicket(unsigned char VPID, const char* buffer);


    //-----------------------------------




#endif //UNPACKER_H
