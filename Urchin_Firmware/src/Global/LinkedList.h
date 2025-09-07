//
// Created by gabri on 5/24/2025.
//

#ifndef LINKEDLIST_H
#define LINKEDLIST_H


#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif



    typedef struct LinkedList{
        void* data;
        struct LinkedList* next;
    }LinkedList;

    /**
    * Creates the next node in a linked list
    * @param data void pointer to your data
    * @return A Node struct
    */
    LinkedList* create_node(const void* data, size_t data_size);

    /**
     * Added the node to start of list
     * @param head start of List
     * @param data void pointer to your data
     */
    int append_node(LinkedList** head, void* data, size_t data_size);


    /**
     * Frees the list
     * @param head Start of List
     */
    void free_list(LinkedList* head);


#ifdef __cplusplus
}
#endif

#endif //LINKEDLIST_H
