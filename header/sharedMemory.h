#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#define MAX_RECORDS 100000

#include "list.h"

typedef struct {
    sem_t *read_mutex;
    sem_t *write_mutex;
    int *readers_started;
    int *readers_completed;
    int *writer_waiting;
} SharedData;

#endif // SHAREDMEMORY_H