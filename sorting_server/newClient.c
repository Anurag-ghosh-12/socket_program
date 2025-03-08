#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Anurag Ghosh 2022CSB101
#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define MAX_ELEMENTS 65535 // 2-byte limit

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    uint16_t numElements;
    int elements[MAX_BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("[-]Socket creation error");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("10.2.65.25"); // 10.2.65.33 in lab

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error("[-]Connection Failed");
    }

    printf("Enter number of elements then elements: ");
    scanf("%hu", &numElements); // Using %hu for uint16_t
    if (numElements == 0 || numElements > MAX_ELEMENTS) {
        printf("Error: Number of elements must be between 1 and 65535.\n");
        close(sockfd);
        return 1;
    }

    // Create buffer and store numElements in first 2 bytes
    char buffer[MAX_BUFFER_SIZE];
    uint16_t numElementsNetworkOrder = htons(numElements);
    memcpy(buffer, &numElementsNetworkOrder, 2);

    // Store the elements after numElements
    for (int i = 0; i < numElements; i++) {
        int temp;
        scanf("%d", &temp);
        int tempNetworkOrder = htonl(temp); 
        memcpy(buffer + 2 + (i * 4), &tempNetworkOrder, 4);
    }

    int totalSize = 2 + (numElements * 4);

    // Sending the buffer
    if (send(sockfd, buffer, totalSize, 0) < 0) {
        error("[-]Failed to send elements");
    }

    // Receiving the sorted elements
    if (recv(sockfd, buffer, totalSize, 0) <= 0) {
        perror("[-]Failed to receive elements");
        close(sockfd);
        return 0;
    }

    // Extracting numElements from first 2 bytes
    memcpy(&numElementsNetworkOrder, buffer, sizeof(uint16_t));
    numElements = ntohs(numElementsNetworkOrder);

    // Extract and print sorted elements
    printf("Received sorted elements:\n");
    for (int i = 0; i < numElements; i++) {
        int receivedElement;
        memcpy(&receivedElement, buffer + 2 + (i * 4), 4);
        printf("%d\t", ntohl(receivedElement));
    }
    printf("\n");

    close(sockfd);
    return 0;
}

