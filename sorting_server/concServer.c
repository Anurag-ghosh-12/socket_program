#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 1235
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
    int clientSocket = *(int*)socketDesc;
    free(socketDesc); // Free the dynamically allocated memory for socket descriptor, because now thread handles it

    int numElements;
    int elements[MAX_BUFFER_SIZE];

    if (recv(clientSocket, &numElements, sizeof(numElements), 0) <= 0) {
        perror("[-]Failed to receive number of elements");
        close(clientSocket);
        pthread_exit(NULL);
    }
    numElements = ntohl(numElements);

    if (recv(clientSocket, elements, numElements * sizeof(int), 0) <= 0) {
        perror("[-]Failed to receive elements");
        close(clientSocket);
        pthread_exit(NULL);
    }

    printf("Received data:\n");
    printf("Number of elements: %d\n", numElements);
    printf("Elements: ");
    for (int i = 0; i < numElements; i++) {
        printf("%d ", elements[i]);
    }
    printf("\n");

    // Sorting the elements
    insertionSort(elements, numElements);

    // Send the sorted elements back to the client
    if (send(clientSocket, elements, numElements * sizeof(int), 0) <= 0) {
        perror("[-]Failed to send sorted elements");
    }

    close(clientSocket);
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
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

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
            pthread_detach(thread_id); // Detach the thread to clean up resources automatically
        }
    }

    close(serverSocket);
    return 0;
}

