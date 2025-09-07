//
// Created by gabri on 5/24/2025.
//
#include "LinkedList.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "Errors.h"



LinkedList* create_node(const void* data,const size_t data_size) {
    LinkedList* node = (LinkedList*) pvPortMalloc(sizeof(LinkedList));

    if (node == NULL){return NULL;}
    node->data = pvPortMalloc(data_size);
    memcpy(node->data,data,data_size);

    node->next = NULL;
    return node;
}


int append_node(LinkedList** head, void* data, size_t data_size) {
    LinkedList* new_node = create_node(data,data_size);

    if(new_node == NULL){return URCHIN_ERROR_FailedToAllocate;}

    if (*head == NULL) {
        *head = new_node;
        return URCHIN_OK;
    }
    LinkedList* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;

    return 0;
}




void free_list(LinkedList* head) {
    while (head != NULL) {
        LinkedList* temp = head;
        head = head->next;
        vPortFree(temp->data);
        vPortFree(temp);
    }
}
