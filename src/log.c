#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "../header/sharedMemory.h"
#include "../header/list.h"

#define SHARED_MEM_NAME "/my_shared_memory"

void initialize_shared_resources(int num_records) {
    int shared_data_size = sizeof(SharedData) + num_records * sizeof(sem_t);
    int shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shm_fd, shared_data_size);

    SharedData *shared_data = (SharedData *)mmap(NULL, shared_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    shared_data->read_mutex = (sem_t *)(shared_data + 1);
    shared_data->write_mutex = shared_data->read_mutex + num_records;
    shared_data->readers_started = (int *)(shared_data->write_mutex + num_records);
    shared_data->readers_completed = shared_data->readers_started + num_records;
    shared_data->writer_waiting = shared_data->readers_completed + num_records;

    for (int i = 0; i < num_records; ++i) {
        sem_init(&shared_data->read_mutex[i], 1, 1);
        sem_init(&shared_data->write_mutex[i], 1, 1);
        shared_data->readers_started[i] = 0;
        shared_data->readers_completed[i] = 0;
        shared_data->writer_waiting[i] = 0;
    }

    printf("Initialized shared resources.\n");

    munmap(shared_data, shared_data_size);
    close(shm_fd);
}

void cleanup_shared_resources(int num_records) {
    shm_unlink(SHARED_MEM_NAME);

    printf("Cleaned up shared resources.\n");
}

SharedData *attach_shared_memory(int num_records) {
    int shared_data_size = sizeof(SharedData) + num_records * sizeof(sem_t);
    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    return (SharedData *)mmap(NULL, shared_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}

void detach_shared_memory(SharedData *shared_data, int num_records) {
    int shared_data_size = sizeof(SharedData) + num_records * sizeof(sem_t);
    munmap(shared_data, shared_data_size);
}

int get_num_records_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int num_records = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            num_records++;
        }
    }

    fclose(file);
    return num_records;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];

    int num_records = get_num_records_from_file(filename);

    initialize_shared_resources(num_records);

    printf("Monitoring shared memory. Press 'z' to exit.\n");

    SharedData *shared_data = attach_shared_memory(num_records);

    // Monitor changes in shared data
    SharedData* current_snapshot;
    SharedData* prev_snapshot;
    int i;
    struct timeval tv;
    int retval;
    fd_set rfds;
    current_snapshot = NULL;
    prev_snapshot = NULL;
    

    while (1) {
        // Capture a snapshot of the shared data
        prev_snapshot = current_snapshot;
        for (i = 0; i < num_records; ++i) {
            sem_wait(&shared_data->read_mutex[i]);
            
            sem_wait(&shared_data->write_mutex[i]);
            printf("here\n");
            current_snapshot->readers_started[i] = shared_data->readers_started[i];
            printf("here\n");
            current_snapshot->readers_completed[i] = shared_data->readers_completed[i];
            printf("here\n");
            current_snapshot->writer_waiting[i] = shared_data->writer_waiting[i];
            printf("here\n");
            sem_post(&shared_data->write_mutex[i]);
            sem_post(&shared_data->read_mutex[i]);
        }
        

        // Check for changes
        int changes_detected = 0;

        for (i = 0; i < num_records; ++i) {
            if (prev_snapshot->readers_started[i] != current_snapshot->readers_started[i] ||
                prev_snapshot->readers_completed[i] != current_snapshot->readers_completed[i] ||
                prev_snapshot->writer_waiting[i] != current_snapshot->writer_waiting[i]) {
                changes_detected = 1;
                break;
            }
        }

        if (changes_detected) {
            printf("Changes in shared data:\n");
            for (i = 0; i < num_records; ++i) {
                printf("Record %d - Readers Started: %d, Readers Completed: %d, Writer Waiting: %d\n",
                       i, current_snapshot->readers_started[i], current_snapshot->readers_completed[i],
                       current_snapshot->writer_waiting[i]);
            }
            printf("-------------\n");
        }

        FD_ZERO(&rfds);
        FD_SET(0, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100 milliseconds

        retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
            break;
        } else if (retval) {
            char input = getchar();
            if (input == 'z') {
                break;
            }
        }
    }

    detach_shared_memory(shared_data, num_records);

    cleanup_shared_resources(num_records);

    return 0;
}
