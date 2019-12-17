#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>

#define MAXCOURSE 250

struct course
{
	char name[50];
	int id;
	int pid;
};
struct course cdata[MAXCOURSE];

void coursereg()
{
	int id,rfd;
	printf("Enter Course Id: \n");
	scanf("%d",&id);
	if(cdata[id].id == -1)
	{	
		cdata[id].id = id;
		printf("Enter Course Name: \n");
		scanf("%s",cdata[id].name);
		if((rfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	              perror("Socket: ");
		struct sockaddr_in rcvgrp,recvaddr;
		struct ip_mreq mreq;
		char ip[20];
		sprintf(ip,"226.1.1.%d",id);
		rcvgrp.sin_addr.s_addr = inet_addr(ip);
		printf("Listening For Group %s\n",ip);
		memcpy(&mreq.imr_multiaddr, &rcvgrp.sin_addr,sizeof(struct in_addr));
		mreq.imr_interface.s_addr = INADDR_ANY;
		int reuse = 1;
		if(setsockopt(rfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
		{
			perror("Setting SO_REUSEADDR error");
			close(rfd);
			exit(1);
		}
		recvaddr.sin_family = AF_INET;
        	recvaddr.sin_addr.s_addr = INADDR_ANY;
        	recvaddr.sin_port = htons(8080);
        	bind(rfd,(struct sockaddr*)&recvaddr, sizeof(recvaddr));
        	setsockopt(rfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
        	int pid = fork();
		if(pid == 0)
        	{
			int sid = cdata[id].id;
			char sname[50];
		        strcpy(sname,cdata[id].name);	
        	        char databuf[25];
        	        while(1)
			{
				read(rfd,databuf,25);
				printf("------------------------------\n");
				printf("Message Received For Subject Id %d And Name %s\n",sid,sname);
        	        	printf("%s\n",databuf);
				printf("------------------------------\n\n");
				printf("Multicast Chat Menu\n\n");
		                printf("Select Action: \n");
        		        printf("1.Register For Course:\n");
                		printf("2.Send Message: \n");
                		printf("3.Deregister For Course:\n");
                		printf("4.Exit\n");
			}
			close(rfd);
        	}
		else
		{
			cdata[id].pid = pid;
			close(rfd);
		}

	}
	else
		printf("Already Registered For The Course\n");

}


void sendmessage()
{
	struct sockaddr_in grpaddr;
	struct ip_mreq mreq;
	int id,sock;
	printf("Enter Course Id: \n");
	scanf("%d",&id);
	if(cdata[id].id == -1)
	{
		printf("Not Registered for this Course\n");
		return;
	}
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		perror("Socket: ");
	char ip[20];
	sprintf(ip,"226.1.1.%d",id);
	grpaddr.sin_family = AF_INET;
        grpaddr.sin_addr.s_addr = inet_addr(ip);
        grpaddr.sin_port = htons(8080);
	char loopch = 1;
        if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
        {
                perror("Setting IP_MULTICAST_LOOP error");
                close(sock);
                exit(1);
        }
        memcpy(&mreq.imr_multiaddr, &grpaddr.sin_addr,sizeof(struct in_addr));
        mreq.imr_interface.s_addr = INADDR_ANY;
        setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
	printf("Enter Message: \n");
	char buf[25];
	scanf("%s",buf);
	sendto(sock,buf,25,0,(struct sockaddr*)&grpaddr, sizeof(grpaddr));
}


int main()
{
	for(int i = 0; i < MAXCOURSE; i++)
		cdata[i].id = -1;
	int sock,rfd;
	int menu;
	struct sockaddr_in grpaddr,recvaddr;
	struct ip_mreq mreq;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	      perror("Socket: ");
	while(1)
	{
		printf("-------------------------\n");
		printf("Multicast Chat Menu\n\n");
		printf("Select Action: \n");
		printf("1.Register For Course:\n");
		printf("2.Send Message: \n");
		printf("3.Deregister For Course:\n");
		printf("4.Exit\n");
		scanf("%d",&menu);
		switch(menu)
		{
			case 1: coursereg();
				break;
			case 2: sendmessage();
				break;
			case 3:
				break;
			case 4: exit(0);
				break;
			default:;
		}
		sleep(1);
	}



	if((rfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
              perror("Socket: ");
	grpaddr.sin_family = AF_INET;
	grpaddr.sin_addr.s_addr = inet_addr("226.1.1.1");
	grpaddr.sin_port = htons(8080);
	memcpy(&mreq.imr_multiaddr, &grpaddr.sin_addr,sizeof(struct in_addr));
	mreq.imr_interface.s_addr = INADDR_ANY;
	setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
	char loopch = 1;
	if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
	{
		perror("Setting IP_MULTICAST_LOOP error");
		close(sock);
		exit(1);
	}
	int reuse = 1;
	if(setsockopt(rfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("Setting SO_REUSEADDR error");
		close(rfd);
		exit(1);
	}
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_addr.s_addr = INADDR_ANY;
        recvaddr.sin_port = htons(8080);
	bind(rfd,(struct sockaddr*)&recvaddr, sizeof(recvaddr));
	setsockopt(rfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));
	if(fork() == 0)
	{
		char databuf[25];
		read(rfd,databuf,25);
		printf("%s\n",databuf);
	}
	else
	{
		printf("Enter Message: \n");
		char buf[25];
		scanf("%s",buf);
		sendto(sock,buf,25,0,(struct sockaddr*)&grpaddr, sizeof(grpaddr));
	}
	return 0;
}
