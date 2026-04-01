//
// Created by gabri on 11/1/2025.
//

#ifndef BRIDGE_H
#define BRIDGE_H

#define BRIDGEMaxNum 253
#define BRIDGEMaxBrand 30
#define BRIDGEMaxModel 10
#define BRIDGEMaxName 50



#ifdef __cplusplus
extern "C" {
#endif

    #pragma pack(push, 1)  // No padding between fields
#include <stdbool.h>

typedef struct{
        unsigned char Num;
        char Brand[BRIDGEMaxBrand];
        char Model[BRIDGEMaxModel];
        char Joint[BRIDGEMaxName];
        int BoundsMin;
        int BoundsMax;
        int AlignmentAngle;
        bool NextNode;
    }BridgeMotor;
#pragma pack(pop)

    extern BridgeMotor BRIDGE[BRIDGEMaxNum];
    extern unsigned char BRIDGEsize;
    //extern SemaphoreHandle_t BRIDGEMutex;

/**
 *
 */
void BridgeInit();

/**
 *
 * @param mem
 */
void AddBridge(BridgeMotor* mem);

/**
 *
 * @param joint
 * @return
 */
BridgeMotor* GetBridge(const char joint[]);

/**
 *
 * @return
 */
int GetBridgeMotorSize();

/**
 *
 * @param index
 * @return
 */
BridgeMotor* GetBridgeN(int index);

#ifdef __cplusplus
}
#endif

#endif //BRIDGE_H
