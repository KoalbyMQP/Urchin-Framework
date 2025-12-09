//
// Created by gabri on 5/24/2025.
//

#include "Packet.h"


#include <string.h>

#include "Coms.h"
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


int Length(Packet* head){
    if (head->Next == NULL) {
        return 1;
    }
    return Length(head->Next)+ 1;
}


Packet* Get(Packet* head, int index) {
    if (head == NULL) {
        // Base case: list is empty or index out of range
        return NULL;
    }
    if (index == 0) {
        // Found the element at the requested index
        return head;
    }
    // Recur for the next node, decrementing index
    return Get(head->Next, index - 1);
}


void PrintPackets(QueueHandle_t Queue ,unsigned int VPID ,Packet* head){
    if (head == NULL) {
        return; // base case: empty list
    }

    // Print current packet
    (void)PrintfToPI(Queue, VPID, "dest:%d, protocol:%d, device:%d, Check:%d, VPID:%d",
                     head->dest,
                     head->protocol,
                     head->device,
                     head->Check,
                     head->VPID);

    // Recurse to next packet
    PrintPackets(Queue, VPID, head->Next);
}