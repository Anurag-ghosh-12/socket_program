#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//Anurag Ghosh 2022CSB101
#define PORT 1235
#define MAX_BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr;
    int numElements;
    int elements[MAX_BUFFER_SIZE];
    char buffer[MAX_BUFFER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("[-]Socket creation error");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//10.2.65.33 used in college lab
    //currently using 127.0.0.1 to test in personal system

    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        error("[-]Connection Failed");
    }

    printf("Enter number of elements: ");
    scanf("%d", &numElements);

    
    printf("Enter the elements: ");
    for (int i = 0; i < numElements; i++) {
        scanf("%d", &elements[i]);
    }

    int numElementsNetworkOrder = htonl(numElements); 
    if (send(sockfd, &numElementsNetworkOrder, sizeof(numElementsNetworkOrder), 0) < 0) {
        error("[-]Failed to send number of elements");
    }

    // Sending the elements
    if (send(sockfd, elements, numElements * sizeof(int), 0) < 0) {
        error("[-]Failed to send elements");
    }
    int result[MAX_BUFFER_SIZE];
    if (recv(sockfd, result, numElements * sizeof(int), 0) <= 0) {
        perror("[-]Failed to receive elements");
        close(sockfd);
        return 0;
    }
    //numElements = ntohl(elements); 
    printf("Received data\n");
    for(int i=0;i<numElements;i++)
    {
        printf("%d\t",result[i]);
    }
    printf("\n");
    close(sockfd);

    return 0;
}
