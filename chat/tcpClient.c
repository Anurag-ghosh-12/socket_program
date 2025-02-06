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
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];
    char msg[MAX_BUFFER_SIZE];

    printf("-----------------------------------------\n");

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("[+] Client Socket Created Successfully\n");

    // Server details
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Connection failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    printf("[+] Connected to the server\n");
    printf("-----------------------------------------\n");
    printf("Welcome to the Anurag Ghosh Chat Server!\n");
    printf("-----------------------------------------\n");

    // Send initial message
    strcpy(msg, "Hello World!!");
    if (send(clientSocket, msg, strlen(msg), 0) < 0) {
        perror("[-] Sending failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    printf("[+] Sent: %s\n", msg);

    // Receive acknowledgment from server
    memset(buffer, 0, sizeof(buffer));
    if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) <= 0) {
        perror("[-] Receiving data failed");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    printf("[+] Received from server: %s\n", buffer);

    printf("\n--- Chat Mode Activated ---\n");
    printf("Type 'Bye' to end the chat.\n");
    printf("---------------------------------------------\n");

    while (1) {
        // Get message from user
        printf("You: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0'; // Remove newline

        // Send message to server
        if (send(clientSocket, msg, strlen(msg), 0) < 0) {
            perror("[-] Sending failed");
            break;
        }

        if (strncmp(msg, "Bye", 3) == 0) {
            printf("[+] Chat session ended by you.\n");
            break;
        }

        // Receive message from server
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) <= 0) {
            perror("[-] Server disconnected or error occurred");
            break;
        }

        printf("Server: %s\n", buffer);
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Server ended the chat session.\n");
            break;
        }
        printf("---------------------------------------------\n");
    }

    // Close socket
    close(clientSocket);
    printf("[+] Connection closed.\n");

    return 0;
}

