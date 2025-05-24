//
// Created by gabri on 3/24/2025.

#ifndef UNPACKER_H
#define UNPACKER_H

#include <string.h>

#define ContextWordSize 15
#define NumOfActions 10

/**
 *@brief
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
    const int (*function)(const char buffer[]);
}Context;



#ifdef __cplusplus
extern "C" {
#endif

    void InitProcessRequestBasic(Context * Basic[]);
    int LED_Action();
    const char* SkipFoward(const char buffer[],unsigned int distance);
    int ProcessRequest(Context Commands[],const char buffer[]);
    int ReqTicket();
    int PunchTicket();
    int CloseTicket();
    int TicketInfo();
    int GetHealth();





#ifdef __cplusplus
}
#endif


#endif //UNPACKER_H
