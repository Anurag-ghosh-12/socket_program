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
    //time() returns the number of seconds elapsed from epoch of 1970
    //currently it rerurns in rawtime variable of type time_t
    
    //but we are interested in time like calendar time -> years date minutes etc. so 
    // localtime is used which takes time_t * as a paramter and returns a struct tm * type variable
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
    while(1)
    {
    
    memset(buffer, '\0', sizeof(buffer));
    
    int recvdbytes=recv(newSocket, buffer, MAX_BUFFER_SIZE, 0);
    if(recvdbytes<=0) {
       printf("[-]Client disconnected. Closing server ... \n");
       break;
    } 
    int requestType = atoi(buffer);  // client's request as an integer

    memset(buffer, '\0', sizeof(buffer));

    switch(requestType) {
        case 1:
            // Query for the time of the server
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            sprintf(buffer,"The current time on server is : %02d:%02d\n",timeinfo->tm_hour,timeinfo->tm_min);
            //strcpy(strinp, asctime(timeinfo));
            //strcat(buffer, "Server Time: ");
            //strcat(buffer, strinp);
            break;
        
        case 2:
            // Query for time and date of the server
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strcpy(strinp, asctime(timeinfo));
            strcat(buffer, "Server Date and Time: ");
            strcat(buffer, strinp);
            break;

        case 3:
            // Query for the server name
            strcpy(buffer, "Server Name: CreativeAG\n");
            break;

        default:
            strcpy(buffer, "Unsupported request\n");
            break;
    }

    // Sending the timer data  to client
    send(newSocket, buffer, strlen(buffer), 0);
    printf("[+]Data sent to client\n");
    }
    // Close sockets
    close(newSocket);
    close(sockfd);

    return 0;
}

