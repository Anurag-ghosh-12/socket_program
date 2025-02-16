#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 1235
#define BUFFER_SIZE 1024

void insertionSort(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);

    new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    // Receive data
    int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        perror("Receive failed");
        close(new_socket);
        exit(EXIT_FAILURE);
    }

    // (first 2 bytes)
    uint16_t numElements;
    memcpy(&numElements, buffer, 2);
    numElements = ntohs(numElements);

    printf("Received %d elements from client.\n", numElements);

    int elements[numElements];
    for (int i = 0; i < numElements; i++) {
        memcpy(&elements[i], buffer + 2 + i * 4, 4);
        elements[i] = ntohl(elements[i]); 
        printf("%d\t",elements[i]);
    }
    printf("\n");
    // Sort the elements
    insertionSort(elements, numElements);

    //  response
    char response[BUFFER_SIZE] = {0};
    uint16_t numElementsNet = htons(numElements);
    memcpy(response, &numElementsNet, 2);

    for (int i = 0; i < numElements; i++) {
        int sortedValue = htonl(elements[i]);
        memcpy(response + 2 + i * 4, &sortedValue, 4);
    }

    // Send back the sorted array
    send(new_socket, response, 2 + numElements * 4, 0);
    printf("Sorted elements sent back to client.\n");

    close(new_socket);
    close(server_fd);
    return 0;
}

