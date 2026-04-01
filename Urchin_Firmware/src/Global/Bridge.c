//
// Created by gabri on 11/1/2025.
//

#include "Bridge.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ESP_PI_Communication/Coms.h"
#include "ESP_PI_Communication/MSGQueue.h"

void BridgeInit() {
    //BRIDGEMutex = xSemaphoreCreateBinaryWithCaps();
}
//rolint: ignore
BridgeMotor BRIDGE[BRIDGEMaxNum];

//rolint: ignore
unsigned char BRIDGEsize = 0;


void AddBridge(BridgeMotor* mem){
    if (mem == NULL) {
        PrintfToPI(DebugQueue, 0, "AddBridge: NULL input");
        return;
    }
    if (BRIDGEsize >= BRIDGEMaxNum) {
        PrintfToPI(DebugQueue, 0, "AddBridge: BRIDGE overflow");
        return;
    }

    memcpy(&BRIDGE[BRIDGEsize], mem, sizeof(BridgeMotor));
    BRIDGEsize++;
}

BridgeMotor* GetBridge(const char joint[]) {
    if (joint == NULL || joint[0] == '\0') {
        PrintfToPI(DebugQueue, 0, "GetBridge: invalid joint (NULL or empty)");
        return NULL;
    }
    for(int i = 0; i < BRIDGEsize; i++) {
        // Log what we are comparing for every entry
        const char* regJoint = BRIDGE[i].Joint;
        PrintfToPI(DebugQueue, 0,
            "Comparing Target: %s to Registered: %s",
            joint, regJoint);

        if (BRIDGE[i].Joint[0] != '\0' && strcmp(joint, BRIDGE[i].Joint) == 0) {
            return &BRIDGE[i];
            }
    }
    PrintfToPI(DebugQueue, 0, "GetBridge: No match found for %s", joint);
    return NULL;
}

int GetBridgeMotorSize() {
    return BRIDGEsize;
}

BridgeMotor* GetBridgeN(int index) {
    if (index < 0 || index >= BRIDGEsize) {
        PrintfToPI(DebugQueue, 0, "GetBridgeN: index out of range");
        return NULL;
    }

    return &BRIDGE[index];
}