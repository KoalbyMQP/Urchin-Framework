//
// Created by gabri on 5/24/2025.
//

#ifndef PACKET_H
#define PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#define HerkulexPacketSize 233
#include "MSGQueue.h"


typedef struct Packet Packet; // Forward declaration

typedef struct Packet{
    /**
     * What output connect the data will take and thus what type of command (local,wrist,... )
     * 0=local
     */
    int dest;

    /**
     * (I2C,Dynamixel,Herkulex)
     * I2C = 0
     * Dynamixel = 1
     * Herkulex = 2
     * CAN = 3
     */
    int protocol;

    /**
     * What Device is being used
     */
    int device;

    /**
     * If this action check out the device
     */
    char Check;

    /**
     * The packet itself
     */
    char contance[HerkulexPacketSize];


    /**
     *
     * This is needed as a return address for commands that want a response
     */
    unsigned char VPID;

    /**
     * The function to be called if there is an ACK back
     * @param VPID The VPID this function was called from
     * @param buffer[] The contents of the ACK
     */
    int (*ACK)(unsigned char VPID ,const char buffer[]);


    /**
     * Next Node
     */
    Packet *Next;

}Packet;

    Packet* CreateNode(int dest, int protocol,int device, char Check, char contance[], unsigned char VPID, int (*ACK)(unsigned char VPID ,const char buffer[]));

    void InsertionHead(Packet** head, Packet* node);


    void RecursiveFree(Packet* head);

    int Length(Packet* head);

    Packet* Get(Packet* head, int index);

    void PrintPackets(QueueHandle_t Queue ,unsigned int VPID ,Packet* head);
#ifdef __cplusplus
}
#endif


#endif //PACKET_H
