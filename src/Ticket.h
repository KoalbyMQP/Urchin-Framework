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


typedef struct{
  int TicketNum;
  LinkedList *packets;
}Ticket;



#ifdef __cplusplus
}
#endif

#endif //TICKET_H
