#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9000
#define MAX_BUFFER_SIZE 1024

int clientSocket;
volatile int chat_active = 1; // Shared flag to control chat session

void* send_message(void* arg) {
    char msg[MAX_BUFFER_SIZE];
    
    while (chat_active) {
        printf("You: ");
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';  // Remove newline

        send(clientSocket, msg, strlen(msg), 0);

        if (strncmp(msg, "Bye", 3) == 0) {
            chat_active = 0;
            break;
        }
    }
    return NULL;
}

void* receive_message(void* arg) {
    char buffer[MAX_BUFFER_SIZE];
    
    while (chat_active) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            perror("[-] Connection closed or error occurred");
            chat_active = 0;
            break;
        }

        printf("\nServer: %s\n", buffer);
        
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Server ended the chat session.\n");
            chat_active = 0;
            break;
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serverAddr;
    pthread_t send_thread, recv_thread;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("[-] Socket creation failed");
        exit(1);
    }
    printf("[+] Client Socket Created Successfully\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Connection failed");
        exit(1);
    }
    printf("[+] Connected to the server\n");
    printf("-----------------------------------------\n");
    printf("Welcome to the Anurag Ghosh Chat Server!\n");
    printf("-----------------------------------------\n");

    // Send initial "Hello World!!" message
    char init_msg[] = "Hello World!!";
    send(clientSocket, init_msg, strlen(init_msg), 0);
    printf("[+] Sent: %s\n", init_msg);

    // Receive initial response
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    printf("[+] Received from server: %s\n", buffer);

    printf("\n--- Chat Mode Activated ---\nType 'Bye' to end the chat.\n");

    // Create sending and receiving threads
    pthread_create(&send_thread, NULL, send_message, NULL);
    pthread_create(&recv_thread, NULL, receive_message, NULL);

    // Wait for both threads to finish
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(clientSocket);
    printf("[+] Connection closed.\n");

    return 0;
}

