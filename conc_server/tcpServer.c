#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 9000
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 5

void handle_client(int clientSocket, struct sockaddr_in clientAddr) {
    char buffer[MAX_BUFFER_SIZE];

    printf("[+] Client connected from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // Receive initial message
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    printf("[+] Received from client: %s\n", buffer);

    // Echo the message back to the client
    send(clientSocket, buffer, strlen(buffer), 0);
    printf("[+] Echoed message back to client\n");

    // Chat begins
    printf("\n--- Chat Mode Activated with %s:%d ---\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) <= 0) {
            printf("[-] Client %s:%d disconnected unexpectedly.\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            break;
        }

        printf("Client [%s:%d]: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
        
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Client [%s:%d] ended the chat session.\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            break;
        }

        printf("You [%s:%d]: ", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        send(clientSocket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Chat session ended by you with client [%s:%d].\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
            break;
        }
    }

    close(clientSocket);
    printf("[+] Connection with client [%s:%d] closed.\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    exit(0);
}

int main() {
    printf("A SIMPLE CHAT SERVER by Anurag Ghosh\n");
    printf("--------------------------------------\n");

    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size = sizeof(clientAddr);
    int client_count = 0;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-] Socket creation failed");
        exit(1);
    }
    printf("[+] Server Socket Created Successfully\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Bind failed");
        exit(1);
    }
    printf("[+] Binding to Port Number %d\n", PORT);

    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("[-] Listen failed");
        exit(1);
    }
    printf("[+] Listening for incoming connections...\n");

    signal(SIGCHLD, SIG_IGN);  // Prevent zombie processes

    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        if (clientSocket < 0) {
            perror("[-] Accept failed");
            continue;
        }

        if (client_count >= MAX_CLIENTS) {
            char *message = "Server busy. Try again later.\n";
            send(clientSocket, message, strlen(message), 0);
            close(clientSocket);
            continue;
        }

        client_count++;

        pid_t pid = fork();
        if (pid == 0) {  // Child process
            close(serverSocket);  // Child doesn't need the server socket
            handle_client(clientSocket, clientAddr);
        } else if (pid > 0) {  // Parent process
            close(clientSocket);  // Parent doesn't need the client socket
        } else {
            perror("[-] Fork failed");
            close(clientSocket);
        }
    }

    close(serverSocket);
    printf("[+] Server shutdown.\n");

    return 0;
}

