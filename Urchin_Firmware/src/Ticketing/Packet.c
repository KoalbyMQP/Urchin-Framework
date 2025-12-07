//
// Created by gabri on 5/24/2025.
//

#include "Packet.h"


#include <string.h>
#include "freertos/FreeRTOS.h"


Packet* CreateNode(int dest, int protocol,int device, char Check, char contance[], unsigned char VPID, int (*ACK)(unsigned char VPID ,const char buffer[])) {
    Packet* node = (Packet*) pvPortMalloc(sizeof(Packet));

    if (node == NULL){return NULL;}
    memset(node,0,sizeof(Packet));

    node->Next = NULL;//Setup next node to be NULL

    node->dest = dest;
    node->protocol = protocol;
    node->device = device;
    node->Check = Check;
    memcpy(node->contance,contance,HerkulexPacketSize);
    node->VPID = VPID;
    node->ACK = ACK;

    return node;
}


void InsertionHead(Packet** head, Packet* node){
    node->Next= *head;
    *head = node;
}

void RecursiveFree(Packet* head){
    if (head->Next == NULL) {
        vPortFree(head);
        return;
    }
    RecursiveFree(head->Next);
    vPortFree(head);
}
