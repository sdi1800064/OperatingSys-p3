#include <stdio.h>
#include <stdlib.h>

#include "list.h"

// Function to create a new node with a given data value
struct LL* createNode(int data) {
    struct LL* newNode = (struct LL*)malloc(sizeof(struct LL));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}


// Function to add a new integer to the linked list in a sorted way
void insertSorted(struct LL** head, int newData) {
    struct LL* newNode = createNode(newData);
    struct LL* current = *head;
    struct LL* prev = NULL;

    // Traverse the list to find the correct position for the new node
    while (current != NULL && current->data < newData) {
        prev = current;
        current = current->next;
    }

    // Insert the new node at the correct position
    if (prev == NULL) {
        // The new node should be the new head of the list
        newNode->next = *head;
        *head = newNode;
    } else {
        // Insert the new node between prev and current
        prev->next = newNode;
        newNode->next = current;
    }
}

// Function to print the elements of the linked list
void printList(struct LL* head) {
    struct LL* current = head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

// Function to free the memory allocated for the linked list
void freeList(struct LL* head) {
    struct LL* current = head;
    while (current != NULL) {
        struct LL* next = current->next;
        free(current);
        current = next;
    }
}