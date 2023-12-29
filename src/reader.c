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

extern char *optarg;

#include "../header/record.h"
#include "../header/sharedMemory.h"

#define MAX_RECORD_SIZE 256

#define SHARED_MEM_NAME "/my_shared_memory"

struct ProgramOptions {
    char* filename;
    int* recids;
    int recid_count;
    int duration;
    int shmid;
};

void printUsage() {
    printf("Usage: ./reader -f filename -l recid[,recid] -d time -s shmid\n");
}

// Συνάρτηση για απελευθέρωση της μνήμης που έχει δεσμευτεί για τα recids
void freeRecids(struct ProgramOptions* options) {
    free(options->recids);
}

// Συνάρτηση για ανάλυση των recids
void parseRecids(struct ProgramOptions* options, const char* recids_str) {
    char* token;
    char* saveptr;

    // Πρώτο κομμάτι
    token = strtok_r(recids_str, ",", &saveptr);

    while (token != NULL) {
        options->recid_count++;
        options->recids = realloc(options->recids, options->recid_count * sizeof(int));

        if (options->recids == NULL) {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }

        options->recids[options->recid_count - 1] = atoi(token);

        // Επόμενο κομμάτι
        token = strtok_r(NULL, ",", &saveptr);
    }
}

struct ProgramOptions parseCommandLine(int argc, char* argv[]) {
    struct ProgramOptions options;
    options.filename = NULL;
    options.recids = NULL;
    options.recid_count = 0;
    options.duration = 0;
    options.shmid = 0;

    int opt;
    while ((opt = getopt(argc, argv, "f:l:d:s:")) != -1) {
        switch (opt) {
            case 'f':
                options.filename = strdup(optarg);
                break;
            case 'l':
                parseRecids(&options, optarg);
                break;
            case 'd':
                options.duration = atoi(optarg);
                break;
            case 's':
                options.shmid = atoi(optarg);
                break;
            default:
                printUsage();
                freeRecids(&options);
                exit(EXIT_FAILURE);
        }
    }

    // Εδώ μπορείτε να προσθέσετε επιπλέον έλεγχους ακεραιότητας και συνέπειας

    return options;
}

void start_read(SharedData *shared_data, int first_record, int last_record) {
    for (int i = first_record; i <= last_record; ++i) {
            sem_wait(&shared_data->read_mutex[i]);
            shared_data->readers_started[i]++;
            sem_post(&shared_data->read_mutex[i]);
            sem_wait(&shared_data->write_mutex[i]);
        }
}

void end_read(SharedData *shared_data, int first_record, int last_record) {
    for (int i = first_record; i <= last_record; ++i) {
            sem_post(&shared_data->write_mutex[i]);
            sem_wait(&shared_data->read_mutex[i]);
            shared_data->readers_completed[i]++;
            if (shared_data->writer_waiting[i] && shared_data->readers_started[i] == shared_data->readers_completed[i]) {
                sem_post(&shared_data->write_mutex[i]);
            }
            sem_post(&shared_data->read_mutex[i]);
        }
}

void displayRecord(const Record *record) {
    printf("ID: %d\n", record->id);
    printf("Last Name: %s\n", record->last_name);
    printf("First Name: %s\n", record->first_name);
    printf("Value: %d\n", record->value);
    printf("----------------\n");
}

int main(int argc, char *argv[]) {
    struct ProgramOptions options = parseCommandLine(argc, argv);

    FILE *file = fopen(options.filename, "rb");

    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, (options.recids[0] - 1) * sizeof(Record), SEEK_SET);

    int shm_fd = shm_open(SHARED_MEM_NAME, O_RDWR, 0666);
    SharedData *shared_data = (SharedData *)mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);


    start_read(shared_data, options.recids[0], options.recids[options.recid_count -1]);

    printf("Recids: ");
    for (int i = 0; i < options.recid_count; i++) {
        printf("%d ", options.recids[i]);
    }
    printf("\n");    sleep(options.duration);

    end_read(shared_data, options.recids[0], options.recids[options.recid_count -1]);
    

    munmap(shared_data, sizeof(SharedData));
    close(shm_fd);

    fclose(file);

    return 0;
}
