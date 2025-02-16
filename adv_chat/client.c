#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SZ 2048
#define NAME_LEN 32

void str_trim_lf(char *arr, int len) {
    for (int i = 0; i < len; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SZ];
    char name[NAME_LEN];

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed.\n");
        return 1;
    }

    printf("Enter your name: ");
    fgets(name, NAME_LEN, stdin);
    str_trim_lf(name, strlen(name));
    send(sockfd, name, NAME_LEN, 0);

    while (1) {
        // Read and display online users table
        bzero(buffer, BUFFER_SZ);
        recv(sockfd, buffer, BUFFER_SZ, 0);
        printf("%s", buffer);

        // Choose recipient
        printf("Enter recipient UID: ");
        fgets(buffer, BUFFER_SZ, stdin);
        send(sockfd, buffer, strlen(buffer), 0);

        // Send message
        printf("Enter message: ");
        fgets(buffer, BUFFER_SZ, stdin);
        send(sockfd, buffer, strlen(buffer), 0);
    }

    close(sockfd);
    return 0;
}
