#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081
#define MAX_BUFFER_SIZE 1024

int clientSocket;
volatile int chat_active = 1;

void str_overwrite_stdout() {
  printf("\r%s",">");
  fflush(stdout);
}

void * receive_message(void * arg)
{
  int clientSocket=*(int *)arg;
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
        str_overwrite_stdout();
        if (strncmp(buffer, "Bye", 3) == 0) {
            printf("[+] Server ended the chat session.\n");
            chat_active = 0;
            break;
        }
    }
    return NULL;
}

void * send_message(void * arg)
{
    char buffer[MAX_BUFFER_SIZE];
    int clientSocket = *(int *) arg;
    while (chat_active) {
        str_overwrite_stdout();
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline
        send(clientSocket, buffer, strlen(buffer), 0);
        if (strncmp(buffer, "Bye", 3) == 0) {
            chat_active = 0;
            break;
        }
    }
    return NULL;
}
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
    serverAddr.sin_addr.s_addr = inet_addr("10.1.76.64");

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

    pthread_t send_thread, receive_thread;
    pthread_create(&send_thread, NULL, send_message, &clientSocket);
    pthread_create(&receive_thread, NULL, receive_message, &clientSocket);
    
    //wait for both treads to finish
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
   
    close(clientSocket);
    printf("[+] Connection closed.\n");

    return 0;
}

