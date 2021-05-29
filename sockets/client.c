#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "timer.h"
#include "node.h"

#define PORT 8080
#define MESSAGE_LIMIT 2000001
   
int main(int argc, char const* argv[]) {
    int sock_fd;
    struct sockaddr_in serv_addr;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\nSocket creation error\n");
        exit(EXIT_FAILURE);
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
       
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("\nInvalid address / Address not supported\n");
        exit(EXIT_FAILURE);
    }
   
    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\nConnection Failed\n");
        exit(EXIT_FAILURE);
    }

    Node *msgTree;
    char message[MESSAGE_LIMIT] = {0};
    size_t msg_length = atoi(argv[2]);
    
    struct timespec before;
    struct timespec after;

    for (int i = 0; i < MESSAGE_LIMIT; i++) {
        message[i] = '0';
    }

    if (strcmp(argv[1], "node") == 0) {
        msgTree = buildTree(msg_length, '0');
    }

    size_t total_sent = 0;

    get_monotonic_time(&before);

    if (strcmp(argv[1], "node") == 0) {
        serialize(msgTree, message);
        msg_length += msg_length + 1;
    }

    while (total_sent != msg_length) {
        ssize_t bytes_sent = send(sock_fd, message + total_sent, msg_length - total_sent, 0);
        if (bytes_sent < 0) {
            perror("Send message error");
            exit(EXIT_FAILURE);
        }
        total_sent += bytes_sent;
    }

    recv(sock_fd, &after, sizeof(after), 0);
    printf("%ld\n", get_elapsed_time_nano(&before, &after));

    if (shutdown(sock_fd, SHUT_RDWR) < 0) {
        perror("Shutdown socket error");
        exit(EXIT_FAILURE);
    }

    if (close(sock_fd) < 0) {
        perror("Close socket error");
        exit(EXIT_FAILURE);
    }

    return 0;
}