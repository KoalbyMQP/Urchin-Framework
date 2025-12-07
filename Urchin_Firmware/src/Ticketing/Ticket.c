//
// Created by gabri on 5/24/2025.
//

#include "Ticket.h"
#include "Packet.h"
void TicketInit() {



}

int TicketFindOpen(Ticket* Tickets[]) {
    for (int i = 0; i < MaxTickets; i++) {
        if (Tickets[i] == NULL){return i;}
    }
    return -1;
}

void TicketClose(Ticket* ticket){
//Recursive free the packets
    RecursiveFree(ticket->Packets);
    vPortFree(ticket);

}
