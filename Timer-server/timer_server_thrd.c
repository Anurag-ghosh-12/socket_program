#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 1234
#define MAX_BUFFER_SIZE 1024

// Structure to pass client socket info to thread
struct client_info {
    int socket;
    struct sockaddr_in addr;
};

// Thread function to handle a client
void *handle_client(void *arg) {
    struct client_info *cinfo = (struct client_info *)arg;
    int clientSocket = cinfo->socket;
    char buffer[MAX_BUFFER_SIZE];
    char strinp[MAX_BUFFER_SIZE];
    time_t rawtime;
    struct tm *timeinfo;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recvdbytes = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);

        if (recvdbytes <= 0) {
            printf("[-]Client disconnected (FD=%d). Closing thread...\n", clientSocket);
            break;
        }

        int requestType = atoi(buffer);
        memset(buffer, 0, sizeof(buffer));

        switch (requestType) {
            case 1:
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                sprintf(buffer, "Current Time: %02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min);
                break;
            case 2:
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                sprintf(buffer, "Date and Time: %s", asctime(timeinfo));
                break;
            case 3:
                strcpy(buffer, "Server Name: CreativeAG\n");
                break;
            default:
                strcpy(buffer, "Unsupported request\n");
                break;
        }

        send(cinfo->socket, buffer, strlen(buffer), 0);
        printf("[+]Response sent to client (FD=%d)\n", cinfo->socket);
    }

    close(clientSocket);
    free(cinfo);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size = sizeof(clientAddr);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Server address setup
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // Start listening
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept clients in a loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&clientAddr, &addr_size);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        printf("[+]Client connected. FD = %d\n", client_fd);

        // Allocate and initialize client info
        struct client_info *cinfo = malloc(sizeof(struct client_info));
        cinfo->socket = client_fd;
        cinfo->addr = clientAddr;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void *)cinfo) != 0) {
            perror("Thread creation failed");
            close(client_fd);
            free(cinfo);
        } else {
            pthread_detach(tid);  // No need to join the thread later
        }
    }

    close(server_fd);
    return 0;
}

