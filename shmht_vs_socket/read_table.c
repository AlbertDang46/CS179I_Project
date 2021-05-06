#include "shmht.h"
#include "timer.h"
#include "hashInt.h"
#include <stdio.h>

char path[] = "./sharedMemory";

int main(int argc, char const* argv[]) {
    size_t length;

    if (strcmp(argv[1], "1") == 0) {
        length = 1;
    } else if (strcmp(argv[1], "100") == 0) {
        length = 100;
    } else if (strcmp(argv[1], "1000") == 0) {
        length = 1000;
    } else if (strcmp(argv[1], "10000") == 0) {
        length = 10000;
    } else if (strcmp(argv[1], "100000") == 0) {
        length = 100000;
    } else if (strcmp(argv[1], "1000000") == 0) {
        length = 1000000;
    }

    struct shmht* sharedMemory = create_shmht(path, 2, 1000000, hashInt, intKeyEq);

    unsigned int flag_key = 0;
    int flag_val = 0;
    size_t flag_val_size = sizeof(flag_val);

    while (flag_val != 1) {
        void* val_pointer = shmht_search(sharedMemory, &flag_key, sizeof(flag_key), &flag_val_size);
        flag_val = val_pointer ? *(int*)val_pointer : 0;
    }

    unsigned int key = 1;

    char* message = (char*)shmht_search(sharedMemory, &key, sizeof(key), &length);
    // printf("Received message: %s\n", message);    

    shmht_remove(sharedMemory, &key, sizeof(key));
    char return_message[] = "Returned!";
    shmht_insert(sharedMemory, &key, sizeof(key), return_message, sizeof(return_message));
    // printf("Sent message: %s\n", return_message);

    shmht_remove(sharedMemory, &flag_key, sizeof(flag_key));
    flag_val = 0;
    shmht_insert(sharedMemory, &flag_key, sizeof(flag_key), &flag_val, sizeof(flag_val));
    // printf("Set lock to: %d for write_table\n", flag_val);

    return 0;
}