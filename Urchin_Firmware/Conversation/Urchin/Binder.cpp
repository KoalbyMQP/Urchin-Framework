//
// Created by gabri on 6/4/26.
//



#include "Conversation/Urchin/UnPacker.h"
#include "Global/Binder.h"



Context* Identifier = nullptr;

int ConversationLength = 0;

Context (*Conversation[MaxVPIDs])[NumOfActions] = {nullptr};

int BinderInit() {

    static Context id_validate = {"Validate", 8, Validate, NULL, 0};
    Identifier = &id_validate;

    static Context Basic[NumOfActions] = {
        {"FormatTicket",12,FormatTicket,NULL,0},
        {"LoadTicket",10,LoadTicket,NULL,0},
        {"PunchTicket",11,PunchTicket,NULL,0},
        {"CloseTicket",11,CloseTicket,NULL,0},
        {"TicketInfo",10,TicketInfo,NULL,0},
        {"GetHealth",9,GetHealth,NULL,0},
        {"Bridge",6,Bridge,NULL,0},
        {"Validate",8,Validate,NULL,0}
    };

    ConversationLength = 8;

    for (int i = 0; i < MaxVPIDs; i++) {
        Conversation[i] = &Basic;
    }

    return 1;
}

