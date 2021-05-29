#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "timer.h"
#include "node.h"

#define PORT 8080
#define MESSAGE_LIMIT 2000001

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

    Node *msgTree;
    char message[MESSAGE_LIMIT] = {0};
    size_t msg_length = atoi(argv[2]);
    
    struct timespec after;

    if (strcmp(argv[1], "node") == 0) {
        msg_length += msg_length + 1;
    }

    size_t total_recv = 0;

    while (total_recv != msg_length) {
        ssize_t bytes_recv = recv(client_fd, message + total_recv, MESSAGE_LIMIT - total_recv, 0);
        if (bytes_recv < 0) {
            perror("Receive message error");
            exit(EXIT_FAILURE);
        }
        total_recv += bytes_recv;
    }

    if (strcmp(argv[1], "node") == 0) {
        deserialize(msgTree, message);
    }

    get_monotonic_time(&after);
    send(client_fd, &after, sizeof(after), 0);

    if (shutdown(client_fd, SHUT_RDWR) < 0) {
        perror("Shutdown socket error");
        exit(EXIT_FAILURE);
    }

    if (close(client_fd) < 0) {
        perror("Close socket error");
        exit(EXIT_FAILURE);
    }

    if (close(server_fd) < 0) {
        perror("Close socket error");
        exit(EXIT_FAILURE);
    }

    return 0;
}