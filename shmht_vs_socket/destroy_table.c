#include "shmht.h"
#include "timer.h"
#include "hashInt.h"
#include <stdio.h>

char path[] = "./sharedMemory";

int main(int argc, char const* argv[]) {
    struct shmht* sharedMemory = create_shmht(path, 2, 1000000, hashInt, intKeyEq);

    int success = shmht_destroy(sharedMemory);
    printf("Destroy Table Success: %d\n", success);

    return 0;
}