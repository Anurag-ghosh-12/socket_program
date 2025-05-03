#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <features.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>

// Function to create raw socket
int createrawsocket(int protocol_to_sniff) {
    int rawsock;
    if ((rawsock = socket(PF_PACKET, SOCK_RAW, htons(protocol_to_sniff))) == -1) {
        perror("Error creating raw socket");
        exit(EXIT_FAILURE);
    }
    return rawsock;
}
/*
struct sockaddr_ll { unsigned short sll_family; /* Always AF_PACKET 
unsigned short sll_protocol; 
/* Protocol -> int sll_ifindex; 
/* Interface number -> unsigned short sll_hatype;
 /* Header type -> unsigned char sll_pkttype; 
 /* Packet type -> unsigned char sll_halen; /* Length of address */ 
 //unsigned char sll_addr[8]; /* Physical layer address */ };
// Function to bind raw socket to network interface
int bindrawsockettointerface(char *device, int rawsock, int protocol) {
    struct sockaddr_ll sll;
    struct ifreq ifr;

    memset(&sll, 0, sizeof(sll));
    memset(&ifr, 0, sizeof(ifr));

    // Get the interface index
    strncpy(ifr.ifr_name, device, IFNAMSIZ);
    if (ioctl(rawsock, SIOCGIFINDEX, &ifr) == -1) {
        perror("Error getting interface index");
        exit(EXIT_FAILURE);
    }

    // Bind the raw socket
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(protocol);

    if (bind(rawsock, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
        perror("Error binding raw socket to interface");
        exit(EXIT_FAILURE);
    }
    return 1;
}

// Function to print received packet in hex format
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

void printpacketinhex(unsigned char *buffer, int length, FILE *log_txt) {
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    fprintf(log_txt,"\nEthernet Header\n");
    fprintf(log_txt,"\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           eth->h_source[0], eth->h_source[1], eth->h_source[2],
           eth->h_source[3], eth->h_source[4], eth->h_source[5]);

    fprintf(log_txt,"\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
           eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
           eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

    fprintf(log_txt,"\t|-Protocol : 0x%04X\n", ntohs(eth->h_proto));
    
    // Check if protocol is IPv4 (0x0800)
    if (ntohs(eth->h_proto) == 0x0800) {
        fprintf(log_txt,"\nInternet Header\n");
        struct sockaddr_in source, dest;
        struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = ip->saddr;
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = ip->daddr;

        fprintf(log_txt, "\t|-Version : %d\n", (unsigned int)ip->version);
        fprintf(log_txt, "\t|-Internet Header Length : %d DWORDS or %d Bytes\n",
                (unsigned int)ip->ihl, ((unsigned int)(ip->ihl)) * 4);
        fprintf(log_txt, "\t|-Type Of Service : %d\n", (unsigned int)ip->tos);
        fprintf(log_txt, "\t|-Total Length : %d Bytes\n", ntohs(ip->tot_len));
        fprintf(log_txt, "\t|-Identification : %d\n", ntohs(ip->id));
        fprintf(log_txt, "\t|-Time To Live : %d\n", (unsigned int)ip->ttl);
        fprintf(log_txt, "\t|-Protocol : %d\n", (unsigned int)ip->protocol);
        fprintf(log_txt, "\t|-Header Checksum : %d\n", ntohs(ip->check));
        fprintf(log_txt, "\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
        fprintf(log_txt, "\t|-Destination IP : %s\n", inet_ntoa(dest.sin_addr));
    }

    // Print packet data in hex format (16 bytes per line)
    //printf("\nPacket Data:\n");
    for (int i = 0; i < length; i++) {
        fprintf(log_txt,"%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) fprintf(log_txt,"\n");  // New line after 16 bytes
    }
    fprintf(log_txt,"\n");
    if (strcmp(inet_ntoa(dest.sin_addr), "10.2.65.33") == 0) {
        printf(">>> Packet for me!\n");

        // Check if destination MAC is broadcast (FF:FF:FF:FF:FF:FF)
        if (eth->h_dest[0] == 0xFF && eth->h_dest[1] == 0xFF &&
            eth->h_dest[2] == 0xFF && eth->h_dest[3] == 0xFF &&
            eth->h_dest[4] == 0xFF && eth->h_dest[5] == 0xFF) {
            
            printf(">>> ARP Broadcast Packet! Sending Response...\n");
            //send_mac_response(raw, eth->h_source, argv[1]);
            printf("ARP Response yet to be built!!!\n");
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <interface> <num_packets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int raw;
    unsigned char packet_buffer[2048];
    int len;
    int packets_to_sniff;
    struct sockaddr_ll packet_info;
    socklen_t packet_info_size = sizeof(packet_info);
    FILE * fp=fopen("pkt_log.txt","w");
    // Create the raw socket
    raw = createrawsocket(ETH_P_ALL);

    // Bind socket to interface
    bindrawsockettointerface(argv[1], raw, ETH_P_ALL);

    packets_to_sniff = atoi(argv[2]);

    // Start sniffing packets
    while (packets_to_sniff--) {
        len = recvfrom(raw, packet_buffer, 2048, 0, (struct sockaddr *)&packet_info, &packet_info_size);
        if (len == -1) {
            perror("Recv from error");
            exit(EXIT_FAILURE);
        } else {
            // Packet received successfully
            printf("-------------------!!!Packet sniffed!!!----------------------\n");
            printpacketinhex(packet_buffer, len,fp);
            printf("------------- ---!!!Written to log file!!!------------------\n");
        }
    }
    fclose(fp);
    close(raw);
    return 0;
}

