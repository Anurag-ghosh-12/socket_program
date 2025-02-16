#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int clientSocket;
volatile int chat_active = 1; // Shared flag to control chat session

void str_overwrite_stdout() {
  printf("\r%s",">");
  fflush(stdout);
}

void* send_message(void* arg) {
    char buffer[MAX_BUFFER_SIZE];
    int clientSocket = *(int *) arg;
    while (chat_active) {
        str_overwrite_stdout();
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline
        send(clientSocket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] You ended the chat session.\n");
            close(clientSocket);
            printf("[+] Server-Client Connection closed.\n");
            
            chat_active = 0;
            break;
        }
    }
    return NULL;
}

void* receive_message(void* arg) {
    char buffer[MAX_BUFFER_SIZE];
    int clientSocket= *(int *)arg;
    while (chat_active) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            perror("[-] Connection closed or error occurred");
            chat_active = 0;
            break;
        }

        printf("\nClient: %s\n", buffer);
        str_overwrite_stdout();
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Client ended the chat session.\n");
            close(clientSocket);
            printf("[+] Server-Client Connection closed.\n");
            
            chat_active = 0;
            break;
        }
    }
    return NULL;
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size = sizeof(clientAddr);
    pthread_t send_thread, recv_thread;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-] Socket creation failed");
        exit(1);
    }
    printf("[+] Server Socket Created Successfully\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("10.2.65.33");

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-] Bind failed");
        exit(1);
    }
    printf("[+] Bound to Port %d\n", PORT);
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

    // Receive initial "Hello World!!" message
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    printf("[+] Received from client: %s\n", buffer);

    // Echo message back to client
    send(clientSocket, buffer, strlen(buffer), 0);
    printf("[+] Echoed message back to client\n");

    printf("\n--- Chat Mode Activated ---\nType 'Bye' to end the chat.\n");

    // Create sending and receiving threads
    pthread_create(&send_thread, NULL, send_message, &clientSocket);
    pthread_create(&recv_thread, NULL, receive_message, &clientSocket);

    // Wait for both threads to finish
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);
    
    close(serverSocket);
    printf("[+] Server closed.\n");
    
    return 0;
}

