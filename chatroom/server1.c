#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int main() {
    printf("A SIMPLE CHAT SERVER by Anurag Ghosh\n");
    printf("--------------------------------------\n");
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size = sizeof(clientAddr);
    char buffer[MAX_BUFFER_SIZE];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-] Socket creation failed");
        exit(1);
    }
    printf("[+] Server Socket Created Successfully\n");

    serverAddr.sin_family = AF_INET;//family- IPV4
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//INADDR_ANY will be for local address

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Bind failed");
        exit(1);
    }
    printf("[+] Binding to Port Number %d\n", PORT);

    if (listen(serverSocket, 5) < 0) {
        perror("[-] Listen failed");
        exit(1);
    }
    printf("[+] Listening for incoming connections...\n");

    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
    if (clientSocket < 0) {
        perror("[-] Accept failed");
        exit(1);
    }
    printf("[+] Client connected from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // Received initial "Hello World!!" message
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    printf("[+] Received from client: %s\n", buffer);

    // Sent the same message back to client
    send(clientSocket, buffer, strlen(buffer), 0);
    printf("[+] Echoed message back to client\n");

    // Chat begins
    printf("\n--- Chat Mode Activated ---\n");
    

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) < 0) {
            perror("[-] Receiving data failed");
            exit(1);
        }
        printf("Client: %s\n", buffer);
        printf("----------------------------------------\n");
        printf("Type 'Bye' to end the chat.\n");
        printf("----------------------------------------\n");
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Client ended the chat session.\n");
            break;
        }

        printf("You: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character if any

        send(clientSocket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "Bye",3) == 0) {
            printf("[+] Chat session ended by you.\n");
            break;
        }
    }
    close(clientSocket);
    close(serverSocket);
    printf("[+] Connection closed.\n");

    return 0;
}
