/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */
/*
 * This sample application is a simple multi-process application which
 * demostrates sharing of queues and memory pools between processes, and
 * using those queues/pools for communication between the processes.
 *
 * Application is designed to run with two processes, a primary and a
 * secondary, and each accepts commands on the commandline, the most
 * important of which is "send", which just sends a string to the other
 * process.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>
#include <rte_common.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_malloc.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_parse_string.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include "mp_commands.h"
#include "timer.h"

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
static const char *_MSG_POOL = "MSG_POOL";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_PRI_2_SEC = "PRI_2_SEC";
struct rte_ring *send_ring, *recv_ring;
struct rte_mempool *message_pool;
volatile int quit = 0;

// added global variables
int arraySize = 1000; // inital Array size. Keeps track of size of Array being sent.
const int increment = 1000;
const int finalArraySize = 1000000;
struct timespec currTime;
const unsigned elmnt_size = 1000000; // size of element

static int
lcore_recv(__rte_unused void *arg)
{
    unsigned lcore_id = rte_lcore_id();
    printf("\nStarting core %u\n", lcore_id);
    while (!quit){
        void *msg;
        if (rte_ring_dequeue(recv_ring, &msg) < 0){
            usleep(5);
            continue;
        }

        // get end time for receiving Array through shared memory
        get_monotonic_time(&currTime);
        struct timespec* currTimeTemp = &currTime;
        long endTime = get_time_nano(currTimeTemp);

        // free-up mempool of Array
        rte_mempool_put(message_pool, msg);

        // flag that waits for RTE_PROC_PRIMARY to write to arrayTimes.csv first
        int flag = 1;
        while (flag) {
            FILE *flagCSV = fopen("flagCSV.txt", "r");
            flag = fgetc(flagCSV) - '0';
            fclose(flagCSV);
        }

        // set flagCSV back to 1
        FILE *flagCSV= fopen("flagCSV.txt", "w");
        fputs("1", flagCSV);
        fclose(flagCSV);

        // write endTime to arrayTimes.csv
        FILE *fpOut = fopen("arrayTimes.csv", "a");
        fprintf(fpOut, "%ld\n", endTime);
        fclose(fpOut);

        // set flagRing to 0 to tell RTE_PROC_PRIMARY to begin creating next array
        FILE *flagRing = fopen("flagRing.txt", "w");
        fputs("0", flagRing);
        fclose(flagRing);

        // increment arraySize to keep track of current Array
        arraySize += increment;

        // quit program once all Arrays's have been created
        if (arraySize > finalArraySize){
            FILE *flagRing = fopen("flagRing.txt", "w");
            fputs("1", flagRing);
            fclose(flagRing);
            printf("\nComplete.\n");
            quit = 1;
        }

    }
    return 0;
}

int
main(int argc, char **argv)
{
    const unsigned flags = 0;
    const unsigned ring_size = 64;
    const unsigned pool_size = 64; // # of elements
    const unsigned pool_cache = 32;
    const unsigned priv_data_sz = 0;
    int ret;
    unsigned lcore_id;
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
    if (rte_eal_process_type() == RTE_PROC_PRIMARY){
        send_ring = rte_ring_create(_PRI_2_SEC, ring_size, rte_socket_id(), flags);
        recv_ring = rte_ring_create(_SEC_2_PRI, ring_size, rte_socket_id(), flags);
        message_pool = rte_mempool_create(_MSG_POOL, pool_size,
                                          elmnt_size, pool_cache, priv_data_sz,
                                          NULL, NULL, NULL, NULL,
                                          rte_socket_id(), flags);
    } else {
        recv_ring = rte_ring_lookup(_PRI_2_SEC);
        send_ring = rte_ring_lookup(_SEC_2_PRI);
        message_pool = rte_mempool_lookup(_MSG_POOL);
    }
    if (send_ring == NULL)
        rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
    if (recv_ring == NULL)
        rte_exit(EXIT_FAILURE, "Problem getting receiving ring\n");
    if (message_pool == NULL)
        rte_exit(EXIT_FAILURE, "Problem getting message pool\n");
    RTE_LOG(INFO, APP, "Finished Process Init.\n");

    // call lcore_recv() on every slave lcore
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        rte_eal_remote_launch(lcore_recv, NULL, lcore_id);
    }


    if (rte_eal_process_type() == RTE_PROC_PRIMARY){

        printf("\nSending Arrays...\n");

        // constructs Array until finalTreeSize is reached
        while(arraySize <= finalArraySize) {

            // flag that waits for SECONDARY_PROC to signal it is ready to recieve next Array
            int flag = 1;
            while (flag) {
                FILE *flagRing = fopen("flagRing.txt", "r");
                flag = fgetc(flagRing) - '0';
                fclose(flagRing);
            }

            // set flagRing back to 1
            FILE *flagRing = fopen("flagRing.txt", "w");
            fputs("1", flagRing);
            fclose(flagRing);

            // construct new Array of size "arraySize"
            char array[1000000] = {0};
            char *message = array;

            for (int i = 0; i < arraySize - 1; i++) {
                message[i] = 'a';
            }
            message[arraySize - 1] = '\0';

            // get start time for sending Array through shared memory
            get_monotonic_time(&currTime);
            struct timespec *currTimeTemp = &currTime;
            long startTime = get_time_nano(currTimeTemp);

            // send message
            void *msg = NULL;
            if (rte_mempool_get(message_pool, &msg) < 0) {
                rte_panic("Failed to get message buffer\n");
            } else {

                strlcpy((char *) msg, message, elmnt_size);

                if (rte_ring_enqueue(send_ring, msg) < 0) {
                    printf("Failed to send message - message discarded\n");
                    rte_mempool_put(message_pool, msg);
                }

            }

            // write start time to arrayTimes.csv
            FILE *fpOut = fopen("arrayTimes.csv", "a");
            fprintf(fpOut, "%d,%ld,", arraySize, startTime);
            fclose(fpOut);

            // increment arraySize for next tree
            arraySize += increment;

            // set flagRing to 0 to tell SECONDARY_PROC to write the end time to arrayTimes.csv
            FILE *flagCSV = fopen("flagCSV.txt", "w");
            fputs("0", flagCSV);
            fclose(flagCSV);
        }

        // once all Arrays have been created exit the program by setting quit = 1
        printf("\nComplete.\n");
        quit = 1;

    }
    else {
        // initial signal from SECONDARY_PROC to tell RTE_PROC_PRIMARY to begin creating Arrays in the mempool
        FILE *flagRing = fopen("flagRing.txt", "w");
        fputs("0", flagRing);
        fclose(flagRing);
        printf("\nReceiving Arrays...\n");
    }

    rte_eal_mp_wait_lcore();
    /* clean up the EAL */
    rte_eal_cleanup();
    return 0;
}

/*

- For the sender a buffer is allocated from the memory pool which gets
  filled with the message data and then it gets queued to the ring.

- And the receiver just dequeues any messages on the receive ring. And then
  frees the buffer space used by the messages back to the memory pool.

- The message pool is in some hugepage.

-l 0-1 -n 4 --proc-type=primary

-l 2-3 -n 4 --proc-type=secondary

*/
