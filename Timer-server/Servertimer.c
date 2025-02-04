#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 1234
#define MAX_BUFFER_SIZE 1024

int main() {
    int sockfd, newSocket;
    struct sockaddr_in serverAddr, newAddr;
    socklen_t addr_size = sizeof(newAddr);
    char buffer[MAX_BUFFER_SIZE];
    char strinp[MAX_BUFFER_SIZE];
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-]Socket creation failed");
        exit(1);
    }
    printf("[+]Server Socket created successfully\n");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[-]Bind failed");
        exit(1);
    }
    printf("[+]Binding to Port Number %d\n", PORT);
   
    if (listen(sockfd, 5) < 0) {
        perror("[-]Listen failed");
        exit(1);
    }
    printf("[+]Listening...\n");

    newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);
    if (newSocket < 0) {
        perror("[-]Accept failed");
        exit(1);
    }
    printf("[+]Client connected\n");
   
    memset(buffer, '\0', sizeof(buffer));
    recv(newSocket, buffer, MAX_BUFFER_SIZE, 0); 
    
    printf("Client data: %s\n", buffer); 
    memset(buffer, '\0', sizeof(buffer));
    printf ( "Current local time and date: %s", asctime (timeinfo) );
    strcpy(strinp, asctime(timeinfo));
   
    strcat(buffer, strinp);
    strcat(buffer, "\n");

    // Sending data to client
    send(newSocket, buffer, strlen(buffer), 0);
    
    printf("[+]Data sent to client\n");
    
    // Closed sockets
    close(newSocket);
    close(sockfd);

    return 0;
}
