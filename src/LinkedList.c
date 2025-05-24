//
// Created by gabri on 5/24/2025.
//
#include "LinkedList.h"

#include <string.h>

#include "freertos/FreeRTOS.h"




LinkedList* create_node(void* data,size_t data_size) {
    LinkedList* node = (LinkedList*) pvPortMalloc(sizeof(LinkedList));
    node->data = pvPortMalloc(data_size);
    memcpy(node->data,data,data_size);

    node->next = NULL;
    return node;
}


void append_node(LinkedList** head, void* data, size_t data_size) {
    LinkedList* new_node = create_node(data,data_size);
    if (*head == NULL) {
        *head = new_node;
        return;
    }
    LinkedList* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
}




void free_list(LinkedList* head) {
    while (head != NULL) {
        LinkedList* temp = head;
        head = head->next;
        vPortFree(temp->data);
        vPortFree(temp);
    }
}
