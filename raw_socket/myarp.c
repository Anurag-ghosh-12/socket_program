#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#define TIMER_ARP 8
/*  ARP Pkt: 42 bytes
[ Ethernet Header ] (14 bytes)
|-- Destination MAC: 6 bytes
|-- Source MAC:      6 bytes
|-- EtherType:       2 bytes (0x0806 = ARP)

[ ARP Header ] (28 bytes)
|-- Hardware Type:       2 bytes (0x0001 = Ethernet)
|-- Protocol Type:       2 bytes (0x0800 = IPv4)
|-- Hardware Addr Len:   1 byte (0x06 = MAC size)
|-- Protocol Addr Len:   1 byte (0x04 = IPv4 size)
|-- Operation:           2 bytes (0x0001 = request)
|-- Sender MAC:          6 bytes
|-- Sender IP:           4 bytes
|-- Target MAC:          6 bytes (zero for request)
|-- Target IP:           4 bytes

*/
 void print_mac_addr(unsigned char *mac) {
     printf("%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
 }
 
 void print_ip_addr(unsigned char *ip) {
     printf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
 }
 
void display_ethernet_header(struct ethhdr *eth) {
    printf("\n=== Ethernet Header ===\n");

    printf("  -> Source MAC      : %02X:%02X:%02X:%02X:%02X:%02X\n",
        eth->h_source[0], eth->h_source[1], eth->h_source[2],
        eth->h_source[3], eth->h_source[4], eth->h_source[5]);

    printf("  -> Destination MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
        eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
        eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

    printf("  -> EtherType       : 0x%04X\n", ntohs(eth->h_proto));
}

 
void display_arp_packet(struct arphdr *arph, unsigned char *data) {
    unsigned char *src_mac = data;
    unsigned char *src_ip  = src_mac + ETH_ALEN;
    unsigned char *dst_mac = src_ip + 4;
    unsigned char *dst_ip  = dst_mac + ETH_ALEN;

    printf("\n=== ARP Packet ===\n");

    printf("  -> Hardware Type     : %u\n", ntohs(arph->ar_hrd));
    printf("  -> Protocol Type     : 0x%04X\n", ntohs(arph->ar_pro));
    printf("  -> MAC Address Length: %u\n", arph->ar_hln);
    printf("  -> IP Address Length : %u\n", arph->ar_pln);
    printf("  -> Operation         : %u (%s)\n", 
        ntohs(arph->ar_op),
        (ntohs(arph->ar_op) == 1 ? "Request" : "Reply"));

    printf("  -> Sender MAC        : ");
    print_mac_addr(src_mac); 
    printf("\n");

    printf("  -> Sender IP         : ");
    print_ip_addr(src_ip); 
    printf("\n");

    printf("  -> Target MAC        : ");
    print_mac_addr(dst_mac); 
    printf("\n");

    printf("  -> Target IP         : ");
    print_ip_addr(dst_ip); 
    printf("\n");
}


void get_interface_info(char *iface, unsigned char *mac, struct in_addr *ip) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;

    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

    // Get my MAC addrss
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl - get mac");
        exit(EXIT_FAILURE);
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

    // Get my IP address
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl - get ip");
        exit(EXIT_FAILURE);
    }
    *ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    close(fd);
}

// Sending ARP Request
void send_arp_request(int sock, char *iface, unsigned char *src_mac, struct in_addr src_ip, struct in_addr target_ip) {
    unsigned char buffer[42];
    struct ethhdr *eth = (struct ethhdr *)buffer;
    struct ether_arp *arp = (struct ether_arp *)(buffer + 14);//because 14B = ethernet hdr

    // Ethernet header making
    memset(eth->h_dest, 0xFF, 6);// Broadcast MAC All FF 's
    memcpy(eth->h_source, src_mac, 6);// Source MAC
    eth->h_proto = htons(ETH_P_ARP);// ARP- 0x0806

    // ARP packet making
    arp->arp_hrd = htons(ARPHRD_ETHER);
    arp->arp_pro = htons(ETH_P_IP);
    arp->arp_hln = 6;//MAC addrs 6 Bytes
    arp->arp_pln = 4;//IPv4 -4 Bytes
    arp->arp_op  = htons(ARPOP_REQUEST);
    memcpy(arp->arp_sha, src_mac, 6); // Sender MAC
    memcpy(arp->arp_spa, &src_ip, 4); // Sender IP
    memset(arp->arp_tha, 0x00, 6); // Target MAC = 0
    memcpy(arp->arp_tpa, &target_ip, 4); // Target IP

    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = if_nametoindex(iface);
    socket_address.sll_halen = ETH_ALEN;
    memset(socket_address.sll_addr, 0xFF, 6);// Broadcast MAC

    if (sendto(sock, buffer, 42, 0,(struct sockaddr *)&socket_address,sizeof(socket_address)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    printf("ARP request sent for IP %s\n", inet_ntoa(target_ip));
}

// Listening for ARP reply
int listen_for_reply(int sock, struct in_addr target_ip, unsigned char *target_mac) {
    unsigned char buffer[2048];
    time_t start = time(NULL);

    while (time(NULL) - start < TIMER_ARP) {
        int len = recvfrom(sock, buffer, sizeof(buffer), MSG_DONTWAIT, NULL, NULL);
        if (len < 0) {
        //MSG_DONTWAIT- work like non-blocking mode- if data not available returns -1 and sets errno
            if (errno == EAGAIN) {
                usleep(100000); 
                continue;
            } 
            else 
            {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        if (ntohs(eth->h_proto) == ETH_P_ARP) {
            struct ether_arp *arp = (struct ether_arp *)(buffer + 14);
            struct in_addr sender_ip;
            memcpy(&sender_ip, arp->arp_spa, 4);
            if (ntohs(arp->arp_op) == ARPOP_REPLY && sender_ip.s_addr == target_ip.s_addr)
             {
               //If the pkt is of reply type and the packet is sent by the system which was our initial target
               //then copying target_mac with appropriate data
		    memcpy(target_mac, arp->arp_sha, 6);
		    printf("\n===============ARP Reply Packet Received================\n");
		    // Showing Ethernet + ARP header
		    display_ethernet_header(eth);
		    display_arp_packet(&(arp->ea_hdr), (unsigned char *)arp->arp_sha);
		    printf("\n========================================================\n");
		    return 1;
	    }
        }
    }
    return 0;
}
 
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <interface> <target_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *iface = argv[1];
    struct in_addr src_ip, target_ip;
    unsigned char src_mac[6], target_mac[6];

    if (!inet_aton(argv[2], &target_ip)) {
        fprintf(stderr, "Invalid target IP address\n");
        return EXIT_FAILURE;
    }
    // get the source MAC and source IP
    get_interface_info(iface, src_mac, &src_ip);
    // Create a raw socket
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sock < 0) {
        perror("Raw Socket");
        return EXIT_FAILURE;
    }
    //Send the ARP Request
    send_arp_request(sock, iface, src_mac, src_ip, target_ip);
    //Receive reply and find mac- within timer expires
    if (listen_for_reply(sock, target_ip, target_mac)) {
        printf("\nReply:MAC of %s is %02X:%02X:%02X:%02X:%02X:%02X\n",
               inet_ntoa(target_ip),
               target_mac[0], target_mac[1], target_mac[2],
               target_mac[3], target_mac[4], target_mac[5]);
    } 
    else //timer expired
    {
        printf("ARP Request failed: No reply received from %s\n", inet_ntoa(target_ip));
    }

    close(sock);
    return 0;
}

