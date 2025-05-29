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
     * What output connect the data will take and thus what type of command (I2C, UART... )
     */
    int dest;

    /**
     * What motor is being used
     */
    int motor;

    /**
     * If this will check out the motor
     */
    char Check;

    /**
     * The packet it self
     */
    char contance[233];
}Packet;



#ifdef __cplusplus
}
#endif


#endif //PACKET_H
