/**********************************  ASSIGNMENT #3 Q2 ***********************************/
/***************************** Aman Sharma  2018H1030137P *******************************/
/***************************** Ankur Vineet 2018H1030144P *******************************/
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXBUF 65536

struct sockaddr_in src, dest;
int raw_sock, icmp_pack = 0, igmp_pack = 0, tcp_pack = 0, udp_pack = 0;
int unk_pack = 0, tot_pack = 0;
FILE *log_file;
void log_ethdr_details(unsigned char*, int);
void log_iphdr_details(unsigned char* , int, int);
void log_icmp_details(unsigned char* , int);
void log_tcp_details(unsigned char* , int);
void log_udp_details(unsigned char* , int);
void process_packet(unsigned char*, int );
int main(int argc, char *argv[]){
    
    unsigned char *data = (unsigned char*)malloc(MAXBUF);
    struct sockaddr saddr;
    struct in_addr in;

    log_file = fopen("log.txt", "w");
    if(!log_file){
        perror("fopen");
        exit(1);
    }
    printf("Receiving packets....\n");

    raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(raw_sock < 0){
        perror("socket");
        exit(2);
    }

    //Sniff Continously
    int len, numbytes;
    for(;;){
        len = sizeof saddr;
        numbytes = recvfrom(raw_sock, data, MAXBUF, 0, &saddr, &len);
        if(numbytes < 0){
            perror("recvfrom");
            close(raw_sock);
            exit(3);
        }
        process_packet(data, numbytes);
    }
    close(raw_sock);
    return 0;
}



void process_packet(unsigned char* data, int numbytes){
    
    struct iphdr *hdr = (struct iphdr*)(data + sizeof(struct ethhdr));
    tot_pack++;
    switch(hdr->protocol){
        case 1 : //ICMP
                icmp_pack++;
                log_icmp_details(data, numbytes);
                break;
        case 2 : //IGMP
                igmp_pack++;
                //log_igmp_details(data, numbytes);
                break;
        case 6: //TCP
                tcp_pack++;
                log_tcp_details(data, numbytes);
                break;
        case 17: //UDP
                udp_pack++;
                log_udp_details(data, numbytes);
                break;
        default: 
                break;
    }
    printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r"
                                ,tcp_pack, udp_pack, icmp_pack, igmp_pack, unk_pack, tot_pack);

}

void log_ethdr_details(unsigned char* Buffer, int Size){
	struct ethhdr *eth = (struct ethhdr *)Buffer;
	fprintf(log_file,"\n");
    fprintf(log_file,"----------------------------ETH_HEADER----------------------------\n");
	fprintf(log_file , "   > Destination MAC    : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
	fprintf(log_file , "   > Source MAC         : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
	//fprintf(log_file,"------------------------------------------------------------------\n");
}

void log_iphdr_details(unsigned char* data, int numbytes, int proto){
    
    unsigned short iphdrlen;
    char *proto_name[5] = {"TCP", "UDP", "ICMP", "IGMP", "UNK"};
    struct iphdr *ip_hdr = (struct iphdr*)data;
    iphdrlen = (ip_hdr->ihl) * 4;

    memset(&src, 0, sizeof src);
    src.sin_addr.s_addr = ip_hdr -> saddr;

    memset(&dest, 0, sizeof dest);
    dest.sin_addr.s_addr = ip_hdr -> daddr;
    log_ethdr_details(data, numbytes);
    //printing ip header information
    fprintf(log_file,"\n");
    fprintf(log_file,"----------------------------IP_HEADER----------------------------\n");
    fprintf(log_file,"   > TTL                  : %d\n",(unsigned int)ip_hdr->ttl);
    fprintf(log_file,"   > Protocol             : %d\n",(unsigned int)ip_hdr->protocol);
    fprintf(log_file,"   > Protocol Name        : %s\n", proto_name[proto]);
    fprintf(log_file,"   > Source IP            : %s\n",inet_ntoa(src.sin_addr));
    fprintf(log_file,"   > Destination IP       : %s\n",inet_ntoa(dest.sin_addr));
    //fprintf(log_file,"------------------------------------------------------------------\n");
}


void log_tcp_details(unsigned char* data, int numbytes){
    
    unsigned short iphdrlen;

    struct iphdr *hdr = (struct iphdr*)(data + sizeof(struct ethhdr));
    iphdrlen = (hdr->ihl) * 4;

    struct tcphdr *tcph = (struct tcphdr*)(data + iphdrlen + sizeof(struct ethhdr)); 

    //printing TCP header information
    log_iphdr_details(data, numbytes, 0);
    fprintf(log_file,"\n");
    fprintf(log_file,"------------------------TCP_HEADER-------------------------------\n");
    fprintf(log_file,"   > Source Port          : %u\n",ntohs(tcph->source));
    fprintf(log_file,"   > Destination Port     : %u\n",ntohs(tcph->dest));
    fprintf(log_file,"   > Window               : %d\n",ntohs(tcph->window));
    fprintf(log_file,"   > Flags                :\n");
    fprintf(log_file,"          -Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
    fprintf(log_file,"          -Finish Flag          : %d\n",(unsigned int)tcph->fin);
    fprintf(log_file,"          -Push Flag            : %d\n",(unsigned int)tcph->psh);
    fprintf(log_file,"          -Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
    fprintf(log_file,"          -Reset Flag           : %d\n",(unsigned int)tcph->rst);
    fprintf(log_file,"----------------------------------------------------------------\n");


}

void log_udp_details(unsigned char* data, int numbytes){
    
    unsigned short iphdrlen;

    struct iphdr *hdr = (struct iphdr*)(data + sizeof(struct ethhdr));
    iphdrlen = (hdr->ihl) * 4;

    struct udphdr *udph = (struct udphdr*)(data + iphdrlen + sizeof(struct ethhdr)); 

    //printing TCP header information
    log_iphdr_details(data, numbytes, 1);
    fprintf(log_file,"\n");
    fprintf(log_file,"------------------------UDP_HEADER-------------------------------\n");
    fprintf(log_file,"   > Source Port          : %u\n",ntohs(udph->source));
    fprintf(log_file,"   > Destination Port     : %u\n",ntohs(udph->dest));
    fprintf(log_file,"----------------------------------------------------------------\n");


}

void log_icmp_details(unsigned char* data, int numbytes){
    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)data;
    iphdrlen = iph->ihl*4;
     
    struct icmphdr *icmph = (struct icmphdr *)(data + iphdrlen + sizeof (struct ethhdr));
    log_iphdr_details(data, numbytes, 2);
    fprintf(log_file,"\n");
    fprintf(log_file,"------------------------ICMP_HEADER-------------------------------\n");
    fprintf(log_file,"   > Type                 : %u\n",ntohs(icmph->type));
     if((unsigned int)(icmph->type) == 11) 
        fprintf(log_file,"  (TTL Expired)\n");
    else if((unsigned int)(icmph->type) == ICMP_ECHOREPLY) 
          fprintf(log_file,"  (ICMP Echo Reply)\n");
    else 
        fprintf(log_file,"  (UNKNOWN)\n");

    fprintf(log_file,"   > Code                 : %u\n",(unsigned int)(icmph->code));
    fprintf(log_file,"   > Checksum             : %u\n",(unsigned int)(icmph->checksum));
    fprintf(log_file,"----------------------------------------------------------------\n");
             
}