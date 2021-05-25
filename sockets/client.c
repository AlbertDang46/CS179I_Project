#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "timer.h"
#include "node.h"

#define PORT 8080
#define MESSAGE_LIMIT 1000000
#define NUM_DATAPOINTS 1000
#define NUM_ITERATIONS 1000
   
int main(int argc, char const* argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\nSocket creation error\n");
        exit(EXIT_FAILURE);
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
       
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("\nInvalid address / Address not supported\n");
        exit(EXIT_FAILURE);
    }
   
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\nConnection Failed\n");
        exit(EXIT_FAILURE);
    }

    long latency_times[NUM_DATAPOINTS] = {0};

    char message[MESSAGE_LIMIT];
    size_t msg_length;
    struct timespec before;
    struct timespec after;

    for (int i = 0; i < MESSAGE_LIMIT; i++)
        message[i] = '0';

    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = 1; i <= NUM_DATAPOINTS; i++) {
            msg_length = i * MESSAGE_LIMIT / NUM_DATAPOINTS;
            size_t total_sent = 0;

            get_monotonic_time(&before);

            while (total_sent != msg_length) {
                ssize_t bytes_sent = send(sockfd, message + total_sent, msg_length - total_sent, 0);
                if (bytes_sent == -1) {
                    perror("Send message error");
                    exit(EXIT_FAILURE);
                }
                total_sent += bytes_sent;
                // printf("Partial bytes sent: %ld\n", total_sent);
            }
            
            // printf("Number of bytes sent: %ld\n", msg_length);
            // printf("Starting time: %ld\n", get_time_nano(&before));

            recv(sockfd, &after, sizeof(after), 0);

            // printf("Ending time: %ld\n", get_time_nano(&after));
            // printf("Time taken: %ld\n", get_elapsed_time_nano(&before, &after));
            // printf("\n");

            latency_times[i - 1] += get_elapsed_time_nano(&before, &after);
        }

        printf("Set %d finished!\n", k);
    }

    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = 1; i <= NUM_DATAPOINTS; i++) {
            
        }
    }

    for (int i = 0; i < NUM_DATAPOINTS; i++) {
        latency_times[i] /= NUM_ITERATIONS;
    }

    for (int i = 1; i <= NUM_DATAPOINTS; i++) {
        printf("Latency of sending %d bytes: %ld\n", i * MESSAGE_LIMIT / NUM_DATAPOINTS, latency_times[i - 1]);
    }

    return 0;
}