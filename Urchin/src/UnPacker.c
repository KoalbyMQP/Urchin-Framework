//
// Created by gabri on 3/24/2025.
//

#include "UnPacker.h"

Context * Basic[NumOfActions];
Context *Load[NumOfActions];
Context *Health[NumOfActions];
Context *Get[NumOfActions];
Context *Sleep[NumOfActions];

void InitProcessRequest() {

    Basic[0]=&Context{"LED",3,LED_Action};
    // "load","Health","Get", "DisableSleep" , "EnableSleep"

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

