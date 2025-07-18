//
// Created by gabri on 3/24/2025.

#ifndef UNPACKER_H
#define UNPACKER_H

#include <string.h>
#include "Global/GLOBAL.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ContextWordSize 15
#define NumOfActions 11









    typedef struct Context Context;  // Forward declaration
    /**
     *@brief A context is a function to be called when a word is seen in message from the PI
     */
    typedef struct Context{
        /**
         * The word or phrase that will signal a kind of action
         */
        const char Name[ContextWordSize];

        /**
         * The length of Name
         */
        const unsigned short int depth;

        /**
         * The function to be called when the name was mentioned
         * @param buffer[] The remaining string after the "Name"
         */
         int (*function)(const char buffer[]);

        /**
         * list contexts to branch to
         */
         Context* branch;

        /**
         * Number of contexts in list
         */
        const unsigned int size;

    }Context;







    //Helpers
    //-------------------------------------------------
    /**
    * Skips the reader forward by the distace
    * @param buffer Char buffer
    * @param distance Distance to truncate
    * @return New begging of string
    */
    const char* SkipFoward(const char buffer[],unsigned int distance);

    /**
    * This goes through a set of Commands calling its fucntion on the buffer
    * @param Commands Array of Contexts
    * @param buffer The char buffer that was receved
    * @return An arror code
    */
    int ProcessRequest(Context Commands[],const uint8_t buffer[]);
    //--------------------------------------------------------------------



    //Context functions
    //-----------------------------------

    /**
    * To be called when the PI is asking for a ticket
    * @param buffer buffer after the "ReqTicket" command (Not Used)
    * @return An error code
    */
    int ReqTicket(const char* buffer);

    /**
    * To be called when the PI is asking for a ticket to be punched(sending a command)
    * @param buffer the constance of the punch
    * @return An error
    */
    int PunchTicket(const char* buffer);

    /**
    * To be called when the Pi wants to close a ticket early
    * @param buffer buffer after the "ReqTicket" command (Not Used)
    * @return An error
    */
    int CloseTicket(const char* buffer);

    /**
    * To be called when the Pi wants to know the status of a Ticket
    * @param buffer Ticket number
    * @return Error code
    */
    int TicketInfo(const char* buffer);

    /**
    * To be called when the Pi wants a health update from the esp32
    * @param buffer the Type of health
    * @return An Error
    */
    int GetHealth(const char* buffer);




    //-----------------------------------

#ifdef __cplusplus
}
#endif




#endif //UNPACKER_H
