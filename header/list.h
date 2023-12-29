#ifndef LIST_H
#define LIST_H

struct LL{
    int data;
    struct LL* next;
};

struct LL* createNode(int data);
void insertSorted(struct LL** head, int newData);
void printList(struct LL* head);
void freeList(struct LL* head);

#endif