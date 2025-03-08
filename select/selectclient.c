#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"  // Change this to the server's IP if needed
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    fd_set read_fds;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Type messages and press Enter to send.\n");

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_socket, &read_fds); // Monitor server messages
        FD_SET(STDIN_FILENO, &read_fds);  // Monitor user input

        // Use select to wait for input from either server or user
        if (select(client_socket + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        // If user typed a message
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(client_socket, buffer, strlen(buffer), 0);
        }

        // If server sent a message
        if (FD_ISSET(client_socket, &read_fds)) {
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Other Client: %s", buffer);
        }
    }

    close(client_socket);
    return EXIT_SUCCESS;
}

