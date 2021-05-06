#include "shmht.h"
#include "timer.h"
#include "hashInt.h"
#include <string.h>
#include <stdio.h>

char path[] = "./sharedMemory";

int main(int argc, char const* argv[]) {
    struct shmht* sharedMemory = create_shmht(path, 2, 1000000, hashInt, intKeyEq);
    
    unsigned int flag_key = 0;
    int flag_val = 0;
    size_t flag_val_size = sizeof(flag_val);

    unsigned int key = 1;

    char one[1] = {0};
    char hundred[100] = {0};
    char thousand[1000] = {0};
    char ten_thousand[10000] = {0};
    char hundred_thousand[100000] = {0};
    char million[1000000] = {0};

    char* message;
    size_t length;

    if (strcmp(argv[1], "1") == 0) {
        message = one;
        length = 1;
    } else if (strcmp(argv[1], "100") == 0) {
        message = hundred;
        length = 100;
    } else if (strcmp(argv[1], "1000") == 0) {
        message = thousand;
        length = 1000;
    } else if (strcmp(argv[1], "10000") == 0) {
        message = ten_thousand;
        length = 10000;
    } else if (strcmp(argv[1], "100000") == 0) {
        message = hundred_thousand;
        length = 100000;
    } else if (strcmp(argv[1], "1000000") == 0) {
        message = million;
        length = 1000000;
    }

    for (int i = 0; i < length - 1; i++)
        message[i] = 'a';

    message[length - 1] = '\0';

    struct timespec before;
    struct timespec after;

    get_monotonic_time(&before);

    shmht_insert(sharedMemory, &key, sizeof(key), message, length);
    // printf("Sent message: %s\n", message);
    
    shmht_remove(sharedMemory, &flag_key, sizeof(flag_key));
    flag_val = 1;
    shmht_insert(sharedMemory, &flag_key, sizeof(flag_key), &flag_val, sizeof(flag_val));
    // printf("Set lock to: %d for read_table\n", flag_val);

    while (flag_val != 0) {
        void* val_pointer = shmht_search(sharedMemory, &flag_key, sizeof(flag_key), &flag_val_size);
        flag_val = val_pointer ? *(int*)val_pointer : 1;
    }

    size_t return_message_length = 9;
    char* return_message = (char*)shmht_search(sharedMemory, &key, sizeof(key), &return_message_length);
    // printf("Received message: %s\n", return_message);

    get_monotonic_time(&after);

    printf("Round Trip %lu Character(s) Time = %1.9f\n", length, get_elapsed_time(&before, &after));

    return 0;
}