#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "../header/record.h"
#include "../header/sharedMemory.h"

#define MAX_RECORD_SIZE 256

#define SHARED_MEM_NAME "/my_shared_memory"

void start_write(SharedData *shared_data, int record_to_write) {
    sem_wait(&shared_data->read_mutex[record_to_write]);
        sem_wait(&shared_data->write_mutex[record_to_write]);
        if (shared_data->readers_started[record_to_write] == shared_data->readers_completed[record_to_write]) {
            sem_post(&shared_data->write_mutex[record_to_write]);
        } else {
            shared_data->writer_waiting[record_to_write] = 1;
            sem_post(&shared_data->write_mutex[record_to_write]);
            sem_wait(&shared_data->read_mutex[record_to_write]);
            shared_data->writer_waiting[record_to_write] = 0;
        }
}

void end_write(SharedData *shared_data, int record_to_write) {
    sem_post(&shared_data->read_mutex[record_to_write]);
}

void displayRecord(const Record *record) {
    printf("ID: %d\n", record->id);
    printf("Last Name: %s\n", record->last_name);
    printf("First Name: %s\n", record->first_name);
    printf("Value: %d\n", record->value);
    printf("----------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 11) {
        fprintf(stderr, "Usage: %s -f filename -l recid -v value -d time -s shmid\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = NULL;
    int recid, value, duration, shmid;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            filename = argv[i + 1];
        } else if (strcmp(argv[i], "-l") == 0) {
            recid = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-v") == 0) {
            value = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            duration = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-s") == 0) {
            shmid = atoi(argv[i + 1]);
        }
    }

    FILE *file = fopen(filename, "rb+");

    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, (recid - 1) * sizeof(Record), SEEK_SET);

    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0666);
    SharedData *shared_data = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    start_write(shared_data, recid);

    printf("Writer writing to record %d\n", recid);
    sleep(duration);

    end_write(shared_data, recid);


    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);

    fclose(file);

    return 0;
}
