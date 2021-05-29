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

#include <time.h>
#include "timer.h"
#include "node.h"

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
#define MESSAGE_LIMIT 1000000
#define NUM_DATAPOINTS 1000
#define DATA_INC 1000

static const char *_MSG_POOL = "MSG_POOL";
static const char *_SEC_2_PRI = "SEC_2_PRI";
static const char *_PRI_2_SEC = "PRI_2_SEC";

struct rte_ring *send_ring, *recv_ring;
struct rte_mempool *message_pool;

static void* copyTreeSHM(Node *root) {
	if (root == NULL) {
		return NULL;
	}

	void *cpRoot = NULL;
	if (rte_mempool_get(message_pool, &cpRoot) < 0) {
		rte_panic("Failed to get message buffer\n");
	}

	((Node *)cpRoot)->key = root->key;
	((Node *)cpRoot)->left = copyTreeSHM(root->left);
	((Node *)cpRoot)->right = copyTreeSHM(root->right);

	return cpRoot;
}

static void deleteTreeSHM(Node *root) {
	if (root == NULL) {
		return;
	}

	deleteTreeSHM(root->left);
	deleteTreeSHM(root->right);
	rte_mempool_put(message_pool, root);	
}

static int
client_recv(struct timespec *before)
{
	void *after;
	while (rte_ring_dequeue(recv_ring, &after) < 0){
		usleep(1);
	}

	printf("%ld\n", get_elapsed_time_nano(before, (struct timespec *)after));

	return 0;
}

static int
server_recv(struct timespec *after, int type)
{
	void *msg;
	while (rte_ring_dequeue(recv_ring, &msg) < 0){
		usleep(1);
	}

	get_monotonic_time(after);

	if (type == 0) {
		rte_mempool_put(message_pool, msg);
	} else if (type == 1) {
		deleteTreeSHM((Node *)msg);
	}

	if (rte_mempool_get(message_pool, &msg) < 0)
		rte_panic("Failed to get message buffer\n");

	((struct timespec *)msg)->tv_sec = after->tv_sec;
	((struct timespec *)msg)->tv_nsec = after->tv_nsec;

	if (rte_ring_enqueue(send_ring, msg) < 0) {
		printf("Failed to send message - message discarded\n");
		rte_mempool_put(message_pool, msg);
	}

	return 0;
}

int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 64;
	unsigned pool_size = 64;
	unsigned elem_size = MESSAGE_LIMIT;
	const unsigned pool_cache = 32;
	const unsigned priv_data_sz = 0;

	if (strcmp(argv[1], "node") == 0) {
		pool_size = MESSAGE_LIMIT;
		elem_size = NODE_SIZE;
	}

	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");

	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		send_ring = rte_ring_create(_PRI_2_SEC, ring_size, rte_socket_id(), flags);
		recv_ring = rte_ring_create(_SEC_2_PRI, ring_size, rte_socket_id(), flags);
		message_pool = rte_mempool_create(_MSG_POOL, pool_size,
				elem_size, pool_cache, priv_data_sz,
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

	Node *msgTree = NULL;
    char message[MESSAGE_LIMIT] = {0};
    size_t msg_length;
    
    struct timespec before;
    struct timespec after;

	for (int i = 0; i < MESSAGE_LIMIT; i++) {
        message[i] = '0';
    }
	
	for (int i = 1; i <= NUM_DATAPOINTS; i++) {
		msg_length = i * DATA_INC;

		if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
			int type = strcmp(argv[1], "char") == 0 ? 0 : 1;
			server_recv(&after, type);
		} else {
			if (strcmp(argv[1], "node") == 0) {
				deleteTree(msgTree);
				msgTree = buildTree(msg_length, '0');
			}

			get_monotonic_time(&before);

			void *msg = NULL;

			if (strcmp(argv[1], "char") == 0) {
				if (rte_mempool_get(message_pool, &msg) < 0) {
					rte_panic("Failed to get message buffer\n");
				}

				strlcpy((char *)msg, message, msg_length);
			} else if (strcmp(argv[1], "node") == 0) {
				msg = copyTreeSHM(msgTree);
			}

			if (rte_ring_enqueue(send_ring, msg) < 0) {
				printf("Failed to send message - message discarded\n");
				rte_mempool_put(message_pool, msg);
			}

			client_recv(&before);
		}
	}

	rte_eal_mp_wait_lcore();

	return 0;
}
