#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 1235
#define MAX_BUFFER_SIZE 1024
//Anurag Ghosh
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

void handleClient(int clientSocket) {
    int numElements;
    int elements[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];
    if (recv(clientSocket, &numElements, sizeof(numElements), 0) <= 0) {
        perror("[-]Failed to receive number of elements");
        close(clientSocket);
        return;
    }
    numElements = ntohl(numElements); 
    
    if (recv(clientSocket, elements, numElements * sizeof(int), 0) <= 0) {
        perror("[-]Failed to receive elements");
        close(clientSocket);
        return;
    }
    printf("Received data:----------\n");
    printf("The number of elements:\t%d\n",numElements);
    printf("The elements are:\t");
    for(int i=0;i<numElements;i++)
    {
        printf("%d\t",elements[i]);
    }
    printf("\n");
    // Sorting the elements
    insertionSort(elements, numElements);
    // Send the sorted elements back to the client
    if (send(clientSocket, elements, numElements * sizeof(int), 0) <= 0) {
        perror("[-]Failed to send sorted elements");
    }

    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addr_size;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[-]Socket creation failed");
        exit(1);
    }
    printf("[+]Server socket created\n");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //10.2.65.33 for using college lab 
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
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
        if (clientSocket < 0) {
            perror("[-]Connection failed");
            continue;
        }
        printf("[+]Client connected\n");
        handleClient(clientSocket);
    }
    close(serverSocket);
    return 0;
}
