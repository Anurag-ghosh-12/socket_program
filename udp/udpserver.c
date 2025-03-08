#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>

#define MAX_BUFFER_SIZE 2048

uint8_t errorcode = 0;

int checkerror_TTL(char ttl) {
    if (ttl & 1 == 0) {
        return 0;
    } else {
      //  printf("%d\n", (int)ttl);
        errorcode = 4;
        return 4;
    }
}

int checkerror_sanity(char *msg, uint32_t PL, int received_length) {
    if(PL<100 )
    {
      errorcode=1;
      return 1;//too small
    }
    if(PL>1472)//1500-28
    {
      errorcode=3;
      return 3;//too large payload length for udp
    }
    if (received_length - 8 != PL) {  
        //printf("Received length: %d, Expected: %u\n", received_length - 8, PL);
        errorcode = 2;
        return 2;
    }
    return 0;
}

int display(int error) {
    if (error == 0) {
        return 0;
    }
    switch (error) {
        case 1:
           printf("Too small payload !!");
           break;
        case 2:
            printf("Message sanity violated - PAYLOAD LENGTH AND PAYLOAD INCONSISTENT\n");
            break;
        case 3:
          break;
        case 4:
            printf("TTL Value not even!!!\n");
            break;
    }
    return 1;
}

void handleclient(int serversocket, struct sockaddr_in *clientaddr, socklen_t addr_size) {
    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);

    int recv_len = recvfrom(serversocket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)clientaddr, &addr_size);
    if (recv_len < 0) {
        perror("recvfrom() failed");
        return;
    }

    //printf("Received from client: %s\n", buffer);

    uint8_t MT;
    memcpy(&MT, buffer, 1);  // MT - Message Type

    uint16_t SN;  // Sequence number
    memcpy(&SN, buffer + 1, 2);
    
    char TTL;
    memcpy(&TTL, buffer + 3, 1);  // TTL - Time to Live
    //TTL=ntohs(TTL);
    
    uint32_t PL;
    memcpy(&PL, buffer + 4, 4);
    PL = ntohl(PL);  

    char msg[MAX_BUFFER_SIZE];
    memcpy(msg, buffer + 8, PL);
    msg[PL] = '\0';

    //printf("Received length: %d, Expected: %u\n", recv_len - 8, PL);

    int error1 = checkerror_sanity(msg, PL, recv_len);

    int error2 = checkerror_TTL(TTL);

    if (display(error1) || display(error2)) {  // If there is an error, send error response
        char errmsg[MAX_BUFFER_SIZE];
        memset(errmsg, 0, MAX_BUFFER_SIZE);
        
        uint8_t var = 1;
        memcpy(errmsg, &var, 1);
        memcpy(errmsg + 1, &SN, 2);
        memcpy(errmsg + 3, &errorcode, 1);

        sendto(serversocket, errmsg, 4, 0, (struct sockaddr *)clientaddr, addr_size);
        return;
    }

    // Send acknowledgment
    const char *response = "Message received";
    sendto(serversocket, response, strlen(response), 0, (struct sockaddr *)clientaddr, addr_size);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Serverport>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int serversocket;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t addr_size = sizeof(clientaddr);

    // Create UDP socket
    serversocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversocket < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr =inet_addr("10.2.65.33");

    // Bind the socket to the port
    if (bind(serversocket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind() failed");
        close(serversocket);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server is running on port %d\n", port);

    // Handle client communication
    while (1) {
        handleclient(serversocket, &clientaddr, addr_size);
    }

    // Close the socket (not reached in normal execution)
    close(serversocket);
    return 0;
}

