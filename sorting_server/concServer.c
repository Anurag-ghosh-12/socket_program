#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 3490
#define MAX_BUFFER_SIZE 1024

void insertionSort(int arr[], int n) {
    int i, key, j;
    for (i = 1; i < n; i++) {
        key = arr[i];
        j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

void* handleClient(void* socketDesc) {//void * because pthread_create expects a void * as 4th argument
    int new_socket = *(int*)socketDesc;
    free(socketDesc); // Free the dynamically allocated memory for socket descriptor, because now thread handles it
    //int elements[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    // Receive data
    int bytes_received = recv(new_socket, buffer, MAX_BUFFER_SIZE, 0);
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
    char response[MAX_BUFFER_SIZE] = {0};
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
    pthread_exit(NULL);
}

int main() {
    int serverSocket, *newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    pthread_t thread_id;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-]Socket creation failed");
        exit(1);
    }
    printf("[+]Server socket created\n");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("10.2.65.33");

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-]Binding failed");
        close(serverSocket);
        exit(1);
    }
    printf("[+]Binding successful\n");

    if (listen(serverSocket, 10) < 0) {
        perror("[-]Listening failed");
        close(serverSocket);
        exit(1);
    }
    printf("[+]Listening for incoming connections...\n");

    while (1) {
        addr_size = sizeof(clientAddr);
        newSocket = malloc(sizeof(int)); // Allocate memory for socket descriptor, for pthread required
        *newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        if (*newSocket < 0) {
            perror("[-]Connection failed");
            free(newSocket);
            continue;
        }
        printf("[+]Client connected\n");

        if (pthread_create(&thread_id, NULL, handleClient, (void*)newSocket) != 0) {
            perror("[-]Thread creation failed");
            free(newSocket);
        } else {
            pthread_detach(thread_id); // Detaching the thread to clean up
        }
    }

    close(serverSocket);
    return 0;
}

