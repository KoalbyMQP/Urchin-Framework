//
// Created by gabri on 5/24/2025.
//

#ifndef TICKET_H
#define TICKET_H

#include "Packet.h"
#include "LinkedList.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
     * @brief Ticket for the processes ques
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
typedef struct{
    /**
     * Ticket number
     */
     int TicketNum;

     /**
      *The commands that will be sent
      */
     Packet *Packets;

}Ticket;



#ifdef __cplusplus
}
#endif

#endif //TICKET_H
