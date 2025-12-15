//
// Created by gabri on 5/24/2025.
//

#ifndef TICKET_H
#define TICKET_H

#include "Packet.h"
#include "Global/LinkedList.h"
#include "TicketNum.h"
#include "Packet.h"

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
     *
     * This is needed as a return address for commands that want a response
     */
    unsigned char VPID;


     /**
      *The commands that will be sent
      */
     Packet *Packets;

}Ticket;


    /**
       * @brief A list of tickets made to store tickets still being loaded and ones that are finished
       */
    extern Ticket TicketDock[sizeof(TickNumType)*StripSize];

    int TicketFindOpen(Ticket* Tickets[]);
    void TicketClose(Ticket* ticket);


#ifdef __cplusplus
}
#endif

#endif //TICKET_H
