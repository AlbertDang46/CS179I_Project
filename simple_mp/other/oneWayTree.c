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
struct timespec currTime;
const unsigned elmnt_size = 24; // size of element in mempool

// binary Tree node
typedef struct Node {
    char key;
    struct Node *left, *right;
} Node;

Node* newNode(char key);
Node* buildTree(unsigned int size, char key);
void memCpyTree(void *msg, Node *root);
void inorder(Node *root);
void destroyTreePool(Node *root);

// creates new node
Node* newNode(char key) {
    Node *temp = malloc(sizeof(Node));
    temp->key = key;
    temp->left = temp->right = NULL;

    return temp;
}

// builds binary Tree of size "size" with key "key"
Node* buildTree(unsigned int size, char key) {
    if (size == 0) {
        return NULL;
    }

    unsigned int remainingSize = size - 1;
    unsigned int halfSize = remainingSize / 2;

    Node *root = newNode(key);
    root->left = buildTree(remainingSize - halfSize, key);
    root->right = buildTree(halfSize, key);

    return root;
}

// memcpy Tree into mempool
void memCpyTree(void *msg, Node *root) {
    if(root != NULL) {
        // copy node to mempool buffer
        memcpy(msg, root, 24);

        if (root->left != NULL) {
            // get new mempool buffer
            void* msgLeft = NULL;
            rte_mempool_get(message_pool, &msgLeft);

            // point node in mempool's left pointer to new mempool buffer
            ((Node*)msg)->left = (Node*) msgLeft;

            memCpyTree(msgLeft, root->left);

        }
        if (root->right != NULL) {
            // get new mempool buffer
            void* msgRight = NULL;
            rte_mempool_get(message_pool, &msgRight);

            // point node in mempool's right pointer to new mempool buffer
            ((Node*)msg)->right = (Node*) msgRight;

            memCpyTree(msgRight, root->right);
        }

        // free old after Tree (no longer needed after it's copied to mempool
        free(root);
    }
}

// traverse Tree inorder
void inorder(Node *root) {
    if (root) {
        inorder(root->left);
        printf("%c\n", root->key);
        inorder(root->right);
    }
}

// free-up mempool of current Tree
void destroyTreePool(Node *root) {
    if (root != NULL) {
        destroyTreePool(root->left);
        destroyTreePool(root->right);
        rte_mempool_put(message_pool, root);
    }
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

        // get end time for receiving Tree through shared memory
        get_monotonic_time(&currTime);
        struct timespec* currTimeTemp = &currTime;
        double endTime = currTimeTemp->tv_sec + currTimeTemp->tv_nsec*1e-9;

        printf("\ncore %u: Received Tree @  %1.9f\n", lcore_id, endTime);

        // print Tree
        printf("\nPrinting Tree...\n");
        Node* root = (Node*) msg;
        inorder(root);

        // free-up mempool of Tree
        destroyTreePool(root);

        printf("\nExiting...\n");
        quit = 1;

    }
    return 0;
}

int
main(int argc, char **argv)
{
    const unsigned flags = 0;
    const unsigned ring_size = 64;
    const unsigned pool_size = 42000; // # of elements
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

        // get Tree size from user
        char enterSend[10];
        printf("\nEnter size of Tree to send: ");
        char* fg_enterSend = fgets(enterSend,10,stdin);
        enterSend[strcspn(enterSend, "\n")] = 0;

        // convert user input to integer
        int treeSize = atoi(enterSend);

        // if input was successful, send Tree through shared memory
        if (treeSize && fg_enterSend) {

            // construct Tree of size "treeSize"
            Node* root = buildTree(treeSize, '0');
            printf("\nPrinting tree...\n");
            inorder(root);

            // get start time for sending Tree through shared memory
            get_monotonic_time(&currTime);
            struct timespec* currTimeTemp = &currTime;
            double startTime = currTimeTemp->tv_sec + currTimeTemp->tv_nsec*1e-9;

            // copy entire Tree to mempool
            void* msg = NULL;
            rte_mempool_get(message_pool, &msg);
            memCpyTree(msg, root);

            // queue only root pointer send_ring
            if (rte_ring_enqueue(send_ring, msg) < 0) {
                printf("Failed to send message - message discarded\n");
                rte_mempool_put(message_pool, msg);
            } else {
                printf("\n%d Node(s) Tree sent @  %1.9f\n", treeSize, startTime);
            }

        }

        // wait for user signal to clean up eal
        char enterExit[10];
        printf("\nPress enter to exit");
        char* fg_enterExit= fgets(enterExit,10,stdin);
        enterExit[strcspn(enterExit, "\n")] = 0;
        if ( fg_enterExit ) {
            printf("Exiting...\n");
            quit = 1;
        }

    }
    else {
        printf("\nWaiting to receive...\n");
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

