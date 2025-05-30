//
// Created by gabri on 5/24/2025.
//

#ifndef PACKET_H
#define PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /**
     * What output connect the data will take and thus what type of command (local,wrist,... )
     */
    int dest;

    /**
     * (I2C,Dynamixel,Herkulex)
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
    char contance[233];
}Packet;



#ifdef __cplusplus
}
#endif


#endif //PACKET_H
