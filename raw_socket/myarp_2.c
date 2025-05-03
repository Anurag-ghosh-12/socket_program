/* 
 * ARP Application using RAW Socket
 * This program implements ARP functionality as a user space application
 * It sends ARP requests and sniffs for ARP responses
 */
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <signal.h>
 #include <errno.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <sys/ioctl.h>
 #include <net/if.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <netinet/if_ether.h>
 #include <net/ethernet.h>
 #include <net/if_arp.h>
 #include <linux/if_packet.h>
 
 #define ARP_TIMEOUT 3  // seconds
 
 // Global variables
 int sockfd;
 int timer_expired = 0;
 unsigned char src_mac[6];
 unsigned char src_ip[4];
 unsigned char target_ip[4];
 struct sockaddr saddr;
 struct sockaddr_ll sock_addr;
 
 // Timeout handler
 void handle_timeout(int sig) {
     timer_expired = 1;
 }
 
 void print_mac(unsigned char *mac) {
     printf("%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
 }
 
 void print_ip(unsigned char *ip) {
     printf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
 }
 
 int get_mac_address(char *interface, unsigned char *mac_addr) {
     struct ifreq ifr;
     int fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (fd < 0) {
         perror("socket");
         return -1;
     }
     memset(&ifr,0,sizeof(ifr)); // clear the structure
 
     strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
     if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
         perror("ioctl");
         close(fd);
         return -1;
     }
 
     memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 6);
     close(fd);
     return 0;
 }
 
 int get_ip_address(char *interface, unsigned char *ip_addr) {
     struct ifreq ifr;
     int fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (fd < 0) {
         perror("socket");
         return -1;
     }
 
     ifr.ifr_addr.sa_family = AF_INET;
     strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
     if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
         perror("ioctl");
         close(fd);
         return -1;
     }
 
     struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
     memcpy(ip_addr, &ipaddr->sin_addr, 4);
     close(fd);
     return 0;
 }
 
 int get_interface_index(char *interface) {
     struct ifreq ifr;
     int fd = socket(AF_INET, SOCK_DGRAM, 0);
     if (fd < 0) {
         perror("socket");
         return -1;
     }
 
     strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
     if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
         perror("ioctl");
         close(fd);
         return -1;
     }
 
     close(fd);
     return ifr.ifr_ifindex;
 }
 
 void ethernet_header_display(struct ethhdr *eth) {
     printf("\nEthernet Header\n");
     printf("\t|-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
         eth->h_source[0], eth->h_source[1], eth->h_source[2],
         eth->h_source[3], eth->h_source[4], eth->h_source[5]);
     printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",
         eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
         eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
     printf("\t|-Protocol            : %d\n", ntohs(eth->h_proto));
 }
 
 void arp_packet_display(struct arphdr *arph, unsigned char *arp_data) {
     unsigned char *sender_mac = arp_data;
     unsigned char *sender_ip = arp_data + ETH_ALEN;
     unsigned char *target_mac = sender_ip + 4;
     unsigned char *target_ip = target_mac + ETH_ALEN;
 
     printf("\nARP Header\n");
     printf("\t|-Hardware Type       : %d\n", ntohs(arph->ar_hrd));
     printf("\t|-Protocol Type       : 0x%04X\n", ntohs(arph->ar_pro));
     printf("\t|-Hardware Length     : %d\n", arph->ar_hln);
     printf("\t|-Protocol Length     : %d\n", arph->ar_pln);
     printf("\t|-Operation           : %d\n", ntohs(arph->ar_op));
 
     printf("\t|-Sender MAC          : ");
     print_mac(sender_mac);
     printf("\n");
 
     printf("\t|-Sender IP           : ");
     print_ip(sender_ip);
     printf("\n");
 
     printf("\t|-Target MAC          : ");
     print_mac(target_mac);
     printf("\n");
 
     printf("\t|-Target IP           : ");
     print_ip(target_ip);
     printf("\n");
 }
 
 int send_arp_request(char *interface) {
     unsigned char buffer[42];
     struct ethhdr *eth = (struct ethhdr *)buffer;
     struct arphdr *arph = (struct arphdr *)(buffer + sizeof(struct ethhdr));
     unsigned char *arp_data = buffer + sizeof(struct ethhdr) + sizeof(struct arphdr);
 
     int ifindex = get_interface_index(interface);
     if (ifindex < 0) return -1;
 
     // Ethernet header
     memset(eth->h_dest, 0xFF, ETH_ALEN);
     memcpy(eth->h_source, src_mac, ETH_ALEN);
     eth->h_proto = htons(ETH_P_ARP);
 
     // ARP header
     arph->ar_hrd = htons(ARPHRD_ETHER);
     arph->ar_pro = htons(ETH_P_IP);
     arph->ar_hln = ETH_ALEN;
     arph->ar_pln = 4;
     arph->ar_op = htons(ARPOP_REQUEST);
 
     // ARP data
     unsigned char *sender_mac = arp_data;
     unsigned char *sender_ip = arp_data + ETH_ALEN;
     unsigned char *target_mac = sender_ip + 4;
     unsigned char *target_ip_ptr = target_mac + ETH_ALEN;
 
     memcpy(sender_mac, src_mac, ETH_ALEN);
     memcpy(sender_ip, src_ip, 4);
     memset(target_mac, 0, ETH_ALEN);
     memcpy(target_ip_ptr, target_ip, 4);
 
     memset(&sock_addr, 0, sizeof(sock_addr));
     sock_addr.sll_family = AF_PACKET;
     sock_addr.sll_protocol = htons(ETH_P_ARP);
     sock_addr.sll_ifindex = ifindex;
     sock_addr.sll_halen = ETH_ALEN;
     memset(sock_addr.sll_addr, 0xFF, ETH_ALEN);
 
     if (sendto(sockfd, buffer, 42, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
         perror("sendto");
         return -1;
     }
 
     printf("\nARP request sent. Waiting for reply...\n");
     return 0;
 }
 
 void process_arp_packet(unsigned char* buffer, int buflen) {
     struct ethhdr *eth = (struct ethhdr *)buffer;
 
     if (ntohs(eth->h_proto) != ETH_P_ARP) return;
 
     struct arphdr *arph = (struct arphdr *)(buffer + sizeof(struct ethhdr));
     if (ntohs(arph->ar_op) != ARPOP_REPLY) return;
 
     unsigned char *arp_data = buffer + sizeof(struct ethhdr) + sizeof(struct arphdr);
     unsigned char *sender_mac = arp_data;
     unsigned char *sender_ip = arp_data + ETH_ALEN;
     unsigned char *target_mac = sender_ip + 4;
     unsigned char *target_ip_ptr = target_mac + ETH_ALEN;
 
     if (memcmp(sender_ip, target_ip, 4) == 0 &&
         memcmp(target_ip_ptr, src_ip, 4) == 0 &&
         memcmp(target_mac, src_mac, ETH_ALEN) == 0) {
 
         alarm(0);
         printf("\n********************ARP Reply Received********************\n");
         ethernet_header_display(eth);
         arp_packet_display(arph, arp_data);
         printf("\nMAC address for IP ");
         print_ip(sender_ip);
         printf(" is ");
         print_mac(sender_mac);
         printf("\n********************************************************\n\n");
 
         close(sockfd);
         exit(0);
     }
 }
 
 int main(int argc, char *argv[]) {
     if (argc != 3) {
         printf("Usage: %s <interface> <target_ip>\n", argv[0]);
         return -1;
     }
 
     char *interface = argv[1];
     char *target_ip_str = argv[2];
 
     if (get_mac_address(interface, src_mac) < 0 ||
         get_ip_address(interface, src_ip) < 0) {
         return -1;
     }
 
     if (inet_pton(AF_INET, target_ip_str, target_ip) != 1) {
         fprintf(stderr, "Invalid IP address format.\n");
         return -1;
     }
 
     sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
     if (sockfd < 0) {
         perror("socket");
         return -1;
     }
 
     signal(SIGALRM, handle_timeout);
     alarm(ARP_TIMEOUT);
 
     if (send_arp_request(interface) < 0) {
         close(sockfd);
         return -1;
     }
 
     while (!timer_expired) {
         unsigned char buffer[65536];
         int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
         if (len < 0) {
             if (errno == EINTR) break;
             perror("recvfrom");
             continue;
         }
 
         process_arp_packet(buffer, len);
     }
 
     if (timer_expired) {
         printf("ARP reply not received within %d seconds.\n", ARP_TIMEOUT);
     }
 
     close(sockfd);
     return 0;
 }
 
