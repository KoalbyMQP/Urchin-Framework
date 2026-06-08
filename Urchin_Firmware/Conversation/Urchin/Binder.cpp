//
// Created by gabri on 6/4/26.
//


#include "Adaptability/Binder.h"
#include "Conversation/Urchin/UnPacker.h"
int BinderInit() {

    static Context id_validate = {"Validate", 8, Validate, NULL, 0};
    Identifier = &id_validate;

    static Context Basic[]={//has max size see Receving.h
        /*{"ReqTicket",9,ReqTicket,NULL,0},*/
        {"FormatTicket",12,FormatTicket,NULL,0},
        {"LoadTicket",10,LoadTicket,NULL,0},
        {"PunchTicket",11,PunchTicket,NULL,0},
        {"CloseTicket",11,CloseTicket,NULL,0},
        {"TicketInfo",10,TicketInfo,NULL,0},
        {"GetHealth",9,GetHealth,NULL,0},
        {"Bridge",6,Bridge,NULL,0},
        {"Validate",8,Validate,NULL,0}
    };

    for(int i=0;i<256;i++) {
        memcpy(Conversation[i], Basic, sizeof(Basic));

    }


return 1;
}
