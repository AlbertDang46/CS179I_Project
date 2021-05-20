#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "timer.h"

#define PORT 8080
#define MESSAGE_LIMIT 1000000
#define NUM_DATAPOINTS 1000

int main(int argc, char const* argv[]) {
    int server_fd;
    int client_fd;
    int opt = 1;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
       
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
       
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Socket options error");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
       
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, addrlen) < 0) {
        perror("Socket bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Socket listen error");
        exit(EXIT_FAILURE);
    }

    if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Socket accept error");
        exit(EXIT_FAILURE);
    }

    char message[MESSAGE_LIMIT] = {0};
    struct timespec after;

    for (int i = 1; i <= NUM_DATAPOINTS; i++) {
        size_t total_recv = 0;

        while (total_recv != i * MESSAGE_LIMIT / NUM_DATAPOINTS) {
            ssize_t bytes_recv = recv(client_fd, message, MESSAGE_LIMIT, 0);
            if (bytes_recv == -1) {
                perror("Receive message error");
                exit(EXIT_FAILURE);
            }
            total_recv += bytes_recv;
            printf("Partial bytes received: %ld\n", total_recv);
        }

        get_monotonic_time(&after);

        printf("Number of bytes received: %ld\n", total_recv);
        printf("Ending time: %ld\n", get_time_nano(&after));
        printf("\n");

        send(client_fd, &after, sizeof(after), 0);
    }

    return 0;
}