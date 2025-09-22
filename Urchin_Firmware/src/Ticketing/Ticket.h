//
// Created by gabri on 5/24/2025.
//

#ifndef TICKET_H
#define TICKET_H

#include "Packet.h"
#include "Global/LinkedList.h"
#include "TicketNum.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MaxTickets 64*StripSize

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
     * ticket type
     */
     char type;

     /**
      *The commands that will be sent
      */
     Packet *Packets;

}Ticket;



#ifdef __cplusplus
}
#endif

#endif //TICKET_H
