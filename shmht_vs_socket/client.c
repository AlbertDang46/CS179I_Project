#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "timer.h"
#define PORT 8080
   
int main(int argc, char const* argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
       
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
   
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char one[1] = {0};
    char hundred[100] = {0};
    char thousand[1000] = "";
    char ten_thousand[10000] = {0};
    char hundred_thousand[100000] = {0};
    char million[1000000] = {0};

    char* message;
    size_t length;

    if (strcmp(argv[1], "1") == 0) {
        length = 1;
        message = one;
    } else if (strcmp(argv[1], "100") == 0) {
        length = 100;
        message = hundred;
    } else if (strcmp(argv[1], "1000") == 0) {
        length = 1000;
        message = thousand;
    } else if (strcmp(argv[1], "10000") == 0) {
        length = 10000;
        message = ten_thousand;
    } else if (strcmp(argv[1], "100000") == 0) {
        length = 100000;
        message = hundred_thousand;
    } else if (strcmp(argv[1], "1000000") == 0) {
        length = 1000000;
        message = million;
    }

    for (int i = 0; i < length - 1; i++)
        message[i] = 'a';

    message[length - 1] = '\0';

    struct timespec before;
    struct timespec after;

    get_monotonic_time(&before);

    send(sock, message, length, 0);
    // printf("Sent message: %s\n", message);

    char return_message[1024] = {0};
    read(sock, return_message, 1024);
    // printf("Received message: %s\n", return_message);

    get_monotonic_time(&after);

    printf("Round Trip Time %lu Character(s) Time = %1.9f\n", length, get_elapsed_time(&before, &after));

    return 0;
}