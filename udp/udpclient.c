#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_PAYLOAD_SIZE 1000

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    int P = atoi(argv[3]);
    int TTL = atoi(argv[4]);
    int NumPackets = atoi(argv[5]);

    if (P < 100 || P > 1000 || TTL < 2 || TTL > 20 || TTL % 2 != 0 || NumPackets < 1 || NumPackets > 50) {
        // Display the input parameters
    printf("Server IP: %s\n", serverIP);
    printf("Server Port: %d\n", serverPort);
    printf("P: %d\n", P);
    printf("TTL: %d\n", TTL);
    printf("NumPackets: %d\n", NumPackets);
        fprintf(stderr, "Invalid input parameters. Ensure P in [100, 1000], TTL in [2, 20] (even), and NumPackets in [1, 50].\n");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);
    char buffer[MAX_PAYLOAD_SIZE + 8];
    char response[8];

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("socket() failed");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        error("Invalid Server IP Address");
    }

    struct timeval start, end;
    double totalRTT = 0.0;

    for (int i = 0; i < NumPackets; i++) {
        uint8_t MT = 1;
        uint16_t SN = htons(i);
        uint8_t ttl = TTL;
        uint32_t PL = htonl(P);
        
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &MT, 1);
        memcpy(buffer + 1, &SN, 2);
        memcpy(buffer + 3, &ttl, 1);
        memcpy(buffer + 4, &PL, 4);
        memset(buffer + 8, 'A', P);//fill with A

        gettimeofday(&start, NULL);
        sendto(sockfd, buffer, 8 + P, 0, (struct sockaddr *)&serverAddr, addrLen);
        
        ssize_t recvLen = recvfrom(sockfd, response, sizeof(response), 0, (struct sockaddr *)&serverAddr, &addrLen);
        gettimeofday(&end, NULL);
        
        if (recvLen < 0) {
            perror("recvfrom() failed");
            continue;
        }

        double rtt = ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_usec - start.tv_usec) / 1000.0);
        totalRTT += rtt;

        uint8_t receivedMT;
        memcpy(&receivedMT, response, 1);
        
        if (receivedMT == 2) { // Error packet
            uint8_t errorCode;
            memcpy(&errorCode, response + 3, 1);
            printf("Packet %d Error: ", (i+1));
            switch (errorCode) {
                case 1: printf("TOO SMALL PACKET RECEIVED\n"); break;
                case 2: printf("PAYLOAD LENGTH AND PAYLOAD INCONSISTENT\n"); break;
                case 3: printf("TOO LARGE PAYLOAD LENGTH\n"); break;
                case 4: printf("TTL VALUE IS NOT EVEN\n"); break;
                default: printf("Unknown Error\n");
            }
        } else {
            printf("Packet %d RTT: %.3f ms\n", (i+1), rtt);
        }
    }

    printf("Average RTT: %.3f ms\n", totalRTT / NumPackets);
    close(sockfd);
    return 0;
}

