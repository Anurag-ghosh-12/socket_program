#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define SERVER_BACKLOG 5
#define BUFFER_SIZE 1024

int setupserver(int port, int backlog);
int accept_new_connection(int server_socket);
void handle_connection(int sender_socket, int other_socket);

int main() {
    int server_socket = setupserver(SERVER_PORT, SERVER_BACKLOG);
    fd_set current_sockets, ready_sockets;
    int client_sockets[2] = {-1, -1}; // To store exactly 2 clients

    FD_ZERO(&current_sockets);//initialise
    FD_SET(server_socket, &current_sockets);//put serversocket inside set
    
    while (true) {
    //select is destructive
        ready_sockets = current_sockets;
        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {
                if (i == server_socket) {
                    // Accept new client
                    int client_socket = accept_new_connection(server_socket);
                    
                    if (client_sockets[0] == -1) {
                        client_sockets[0] = client_socket;
                    } else if (client_sockets[1] == -1) {
                        client_sockets[1] = client_socket;
                    } else {
                        printf("Maximum 2 clients supported. Closing extra connection.\n");
                        close(client_socket);
                        continue;
                    }

                    FD_SET(client_socket, &current_sockets);
                } else {
                    // Handle client message forwarding
                    int other_socket = (client_sockets[0] == i) ? client_sockets[1] : client_sockets[0];
                    handle_connection(i, other_socket);

                    if (other_socket == -1) {
                        FD_CLR(i, &current_sockets);
                        close(i);
                        client_sockets[0] = (client_sockets[0] == i) ? -1 : client_sockets[0];
                        client_sockets[1] = (client_sockets[1] == i) ? -1 : client_sockets[1];
                    }
                }
            }
        }
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

// Setup server socket
int setupserver(int port, int backlog) {
    int server_fd;
    struct sockaddr_in server_addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, backlog) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);
    return server_fd;
}

// Accept a new client connection
int accept_new_connection(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
    
    if (client_fd < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("New client connected: socket %d\n", client_fd);
    return client_fd;
}

// Read message from sender and forward to receiver
void handle_connection(int sender_socket, int receiver_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read = read(sender_socket, buffer, sizeof(buffer));
    
    if (bytes_read <= 0) {
        printf("Client %d disconnected\n", sender_socket);
        close(sender_socket);
        return;
    }

    buffer[bytes_read] = '\0';
    printf("Client %d: %s\n", sender_socket, buffer);

    if (receiver_socket != -1) {
        send(receiver_socket, buffer, bytes_read, 0);
        printf("Forwarded to client %d\n", receiver_socket);
    } else {
        printf("No client available to receive message\n");
    }
}

