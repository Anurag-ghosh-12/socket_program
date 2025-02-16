#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
//Anurag Ghosh 2022CSB101
#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];
    char msg[MAX_BUFFER_SIZE];
    printf("-----------------------------------------\n");
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("[-] Socket creation failed");
        exit(1);
    }
    printf("[+] Client Socket Created Successfully\n");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("10.2.79.114");

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Connection failed");
        exit(1);
    }
    printf("[+] Connected to the server\n");
    printf("-----------------------------------------\n");
    printf("Welcome to the Anurag Ghosh Chat Server!\n");
    printf("-----------------------------------------\n");
    printf("-----------------------------------------\n");
    

    // Initially "Hello World!!" message during connection phase
    strcpy(msg, "Hello World!!");
    send(clientSocket, msg, strlen(msg), 0);
    printf("[+] Sent: %s\n", msg);

    memset(buffer, 0, sizeof(buffer));
    if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) < 0) {
        perror("[-] Receiving data failed");
        exit(1);
    }
    printf("[+] Received from server: %s\n", buffer);
    printf("\n--- Chat Mode Activated ---\n");
    printf("Type 'Bye' to end the chat.\n\n");
    printf("---------------------------------------------\n");

    while(1) {
        printf("Enter the message:\n");
        printf("---------------------------------------------\n");
        printf("You: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';  // Removed newline character if any 

        send(clientSocket, msg, strlen(msg), 0);
        printf("---------------------------------------------\n");
        
        if (strncmp(msg, "Bye",3) == 0) //first 3 char match then close connection
        {
            printf("[+] Chat session ended by you.\n");
            break;
        }
        printf("\nSent to the server...waiting for reply...\n");
        printf("---------------------------------------------\n");
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) < 0) {
            perror("[-] Receiving data failed");
            exit(1);
        }
        printf("Server: %s\n", buffer);

        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Server ended the chat session.\n");
            break;
        }
        printf("---------------------------------------------\n");
    }

    close(clientSocket);
    printf("[+] Connection closed.\n");

    return 0;
}

