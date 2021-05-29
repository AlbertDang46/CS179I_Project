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
#include "time.h"
#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
static const char *_MSG_POOL = "MSG_POOL";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_PRI_2_SEC = "PRI_2_SEC";
struct rte_ring *send_ring, *recv_ring;
struct rte_mempool *message_pool;
volatile int quit = 0;
// added stuff
int length = 1; // keeps track of size of array being sent. Mainly used for printing statement
struct timespec before, after;
const unsigned elmnt_size = 1000000; // size of element

void get_monotonic_time(struct timespec* ts);
double get_elapsed_time(struct timespec* beforeT, struct timespec* afterT);

void get_monotonic_time(struct timespec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}


double get_elapsed_time(struct timespec* beforeT, struct timespec* afterT) {
    double deltat_s  = afterT->tv_sec - beforeT->tv_sec;
    double deltat_ns = afterT->tv_nsec - beforeT->tv_nsec;
    return deltat_s + deltat_ns*1e-9;
}


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
        printf("\ncore %u: Message received.\n", lcore_id);
        // wait for user signal to exit program
        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
            // get final time measurement
            get_monotonic_time(&after);
            printf("\nRound Trip %d Byte(s) Time = %1.9f\n", length, get_elapsed_time(&before, &after));
        }
        else {

            // send message back
            char sendThis[1] = {'\0'};
            void *msgBack = NULL;

            if (rte_mempool_get(message_pool, &msgBack) < 0) {
                rte_panic("Failed to get message buffer\n");
            }
            else {

                strlcpy((char *) msgBack, sendThis, 8);

                if (rte_ring_enqueue(send_ring, msgBack) < 0) {
                    printf("Failed to send message - message discarded\n");
                    rte_mempool_put(message_pool, msgBack);
                }
                else {
                    printf("\nSent back confirmation message.\n");
                }
            }

        }

        rte_mempool_put(message_pool, msg);
        quit = 1;

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


        // wait for user signal to send message
        char enterSend[10];
        printf("\nHow many bytes to send: ");
        char* fg_enterSend = fgets(enterSend,10,stdin);
        enterSend[strcspn(enterSend, "\n")] = 0;

        // create message
        char one[1] = {0};
        char hundred[100] = {0};
        char thousand[1000] = {0};
        char ten_thousand[10000] = {0};
        char hundred_thousand[100000] = {0};
        char million[1000000] = {0};

        char* message = NULL;

        if (strcmp(enterSend, "1") == 0 && fg_enterSend) {
            message = one;
            length = 1;
        } else if (strcmp(enterSend, "100") == 0 && fg_enterSend) {
            message = hundred;
            length = 100;
        } else if (strcmp(enterSend, "1000") == 0 && fg_enterSend) {
            message = thousand;
            length = 1000;
        } else if (strcmp(enterSend, "10000") == 0 && fg_enterSend) {
            message = ten_thousand;
            length = 10000;
        } else if (strcmp(enterSend, "100000") == 0 && fg_enterSend) {
            message = hundred_thousand;
            length = 100000;
        } else if (strcmp(enterSend, "1000000") == 0 && fg_enterSend) {
            message = million;
            length = 1000000;
        }
        else {
            printf("\nWrong Input.");
        }


        for (int i = 0; i < length - 1; i++) {
            message[i] = 'a';
        }
        message[length - 1] = '\0';

        // send message
        void *msg = NULL;

        get_monotonic_time(&before);

        if (rte_mempool_get(message_pool, &msg) < 0) {
            rte_panic("Failed to get message buffer\n");
        }
        else {

            strlcpy((char *) msg, message, elmnt_size);

            if (rte_ring_enqueue(send_ring, msg) < 0) {
                printf("Failed to send message - message discarded\n");
                rte_mempool_put(message_pool, msg);
            }
            else {
                printf("\n%d Byte(s) sent.\n", length);
            }

        }

    }

    rte_eal_mp_wait_lcore();
    /* clean up the EAL */
    rte_eal_cleanup();
    return 0;
}

/*
 *
- For the sender a buffer is allocated from the memory pool which gets
  filled with the message data and then it gets queued to the ring.

- And the receiver just dequeues any messages on the receive ring. And then
  frees the buffer space used by the messages back to the memory pool.

- The message pool is in some hugepage.

-l 0-1 -n 4 --proc-type=primary

-l 2-3 -n 4 --proc-type=secondary

*/

