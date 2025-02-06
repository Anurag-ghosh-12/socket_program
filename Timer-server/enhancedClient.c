#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 1234
#define MAX_BUFFER_SIZE 1024

void displayMenu() {
    printf("\n==========WELCOME==============\n");
    printf("       USER MENU\n");
    printf("==============================\n");
    printf("1. Request Current Time\n");
    printf("2. Request Time and Date\n");
    printf("3. Request Server Name\n");
    printf("4. Exit\n");
    printf("==============================\n");
    printf("Enter your choice: ");
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_BUFFER_SIZE];
    int choice;

    // Creating the socket
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
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

    do {
        displayMenu();
        scanf("%d", &choice);
        getchar(); // Clearing the newline character

        switch (choice) {
            case 1:
            case 2:
            case 3:
                // Send the selected choice to the server
                sprintf(buffer, "%d", choice);
                
                if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
                    perror("[-] Sending data failed");
                    exit(1);
                }
                printf("[+] Request sent to server\n");

                memset(buffer, 0, MAX_BUFFER_SIZE); // Clearing the buffer
                if (recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0) < 0) {
                    perror("[-] Receiving data failed");
                    exit(1);
                }
                printf("\n[+] Response from server: %s\n", buffer);
                break;

            case 4:
                printf("[+] Exiting the application...\n");
                break;

            default:
                printf("[-] Invalid choice. Please try again.\n");
                break;
        }

    } while (choice != 4);

    // Close the connection
    close(clientSocket);
    printf("[+] Connection closed\n");

    return 0;
}

