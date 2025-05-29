//
// Created by gabri on 3/24/2025.

#ifndef UNPACKER_H
#define UNPACKER_H

#include <string.h>
#include "GLOBAL.h"

#define ContextWordSize 15
#define NumOfActions 10





#ifdef __cplusplus
extern "C" {
#endif


    /**
     *@brief used to store
     *@details
     */
    typedef struct {
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
         */
        int (*function)(const char buffer[]);
    }Context;



    int LED_Action();
    const char* SkipFoward(const char buffer[],unsigned int distance);
    int ProcessRequest(Context Commands[],const uint8_t buffer[]);
    int ReqTicket(const char* buffer);
    int PunchTicket(const char* buffer);
    int CloseTicket(const char* buffer);
    int TicketInfo(const char* buffer);
    int GetHealth(const char* buffer);





#ifdef __cplusplus
}
#endif


#endif //UNPACKER_H
