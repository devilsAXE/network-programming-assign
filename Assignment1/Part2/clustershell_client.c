/***************************************\ 
*        NP Assignment 1 - Part 2       *
*        Ankur Vineet (2018H1030144P)   *
*        Aman Sharma  (2018H1030137P)   *
\***************************************/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<pwd.h>
#include<sys/socket.h>
#include<string.h>
#define MAXLINE 2048
#define MAXCMND 25
size_t buflen = 50;
int nodeindex = 0;
char **iplist = 0;
char *ip;

//Configuration File Format Example
//n1 <ip>
//n2 <ip>

//Implement nodes command using connect()
int checknode(char *ipaddr)
{
	int sockfd;
	struct sockaddr_in serveraddr;
        if((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
                perror("Socket: ");
        serveraddr.sin_port = 8000;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(ipaddr);
	if( connect(sockfd,(struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		close(sockfd);
		return 0;
	}
	else
	{
		close(sockfd);
		return 1;
	}

}

//Send Data Received by the previous command
void senddata(int sockfd)
{
	int filefd = open("./temp.txt",O_RDONLY);
	char ch[MAXLINE];
	memset(ch,0,sizeof(ch));
	int n;
	while(n = read(filefd,ch,MAXLINE))
		write(sockfd,ch,n);
	//write(sockfd,'\0',1);
	//close(sockfd);
	shutdown(sockfd,SHUT_WR);
	close(filefd);
}

//Send Command to be executed if required data is also sent using above function
int sendcommand(char *ipaddr,char *buf,int first,int last,int append)
{
	int sockfd,filefd;
	//filefd = open("./temp.txt",O_CREAT|O_TRUNC,0666);
	//close(filefd);
	char recvline[MAXLINE];
	//printf("first %d last %d\n",first,last);
	struct sockaddr_in serveraddr;
        if((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
                perror("Socket: ");
        serveraddr.sin_port = 8000;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(ipaddr);
	if( connect(sockfd,(struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		printf("Node Offline\n");
                //perror("connect: ");
		close(sockfd);
		return -2;
	}
	else
	{
		while(*buf != '.')
			buf++;
		buf++;
		struct passwd *pw = getpwuid(getuid());
		char *homedir = pw->pw_dir;
		write(sockfd,homedir,50);
		write(sockfd,buf,buflen);
		if(first == 0)
			senddata(sockfd);
		int n;
		if(append == 0)
			filefd = open("./temp.txt",O_CREAT|O_TRUNC,0666);
		close(filefd);
		for(int i = 0; i < MAXLINE; i++)
			recvline[i] = 0;
		if(append == 0)
			filefd = open("./temp.txt",O_WRONLY);
		if(append == 1)
			filefd = open("./temp.txt",O_WRONLY|O_APPEND);
		while((n = read(sockfd,recvline,MAXLINE)) > 0)
        	{
			if(last == 0)
				write(filefd,recvline,n);
                	recvline[n] = '\0';
			if(last == 1)
			{
				printf("%s",recvline);
				fflush(stdout);
			}
        	}
		close(filefd);
	}
	close(sockfd);
	return 0;
}

//Read IP Address from configuration file
void parseconf(char *path)
{
	int fd = open(path,O_CREAT|O_RDONLY,0644);
	char ch;
	while(read(fd,&ch,1))
	{
		printf("%c",ch);
		if(ch == '\n')
			nodeindex++;
	}
	printf("No. of Nodes: %d\n",nodeindex);
	if(nodeindex == 0)
		exit(0);
	close(fd);
	iplist = (char**) calloc(nodeindex, sizeof(char*));
	for (int i = 0; i < nodeindex; i++ )
    		iplist[i] = (char*) calloc(16, sizeof(char));
	fd = open("./conf",O_RDONLY);
	int i = 0,j = 0;
	while(read(fd,&ch,1))
	{
		if(ch == '\n')
			iplist[i][j] = '\0';
		else
			iplist[i][j] = ch;
		j++;
		if(ch == '\n')
		{
			i++;
			j = 0;
		}
	}
	//for(i = 0 ; i < nodeindex; i++)
	//	printf("%s",iplist[i]);
}

//Extract IP Address from the Node ID
void getip(char *buf)
{
	char *cbuf = calloc(10,sizeof(char));
	int i = 0;
	while(buf[i] != '.' && buf[i] != ' ')
	{
		cbuf[i] = buf[i];
		i++;
	}
	cbuf[i+1] = '\0';
	printf("\n%s --> ",cbuf);
	for(int i = 0; i < nodeindex; i++)
	{
		ip = strstr(iplist[i],cbuf);
		if(ip != NULL)
		{
			while(*ip != ' ')
			 	ip = ip + 1;
			ip++;
			return ;
		}
	}
}
// Manage Execution of Single Command Across Nodes
int executecommand(int len, char *buf,int first,int last)
{
 	if(len > 1 && buf[1] == '*')
	{
		printf("Executing on All Nodes\n");
		for(int i = 0 ; i < nodeindex; i++)
		{
			getip(iplist[i]);
			printf("%s\n",ip);
			if(i == 0)
				sendcommand(ip,buf,first,last,0);
			else
				sendcommand(ip,buf,first,last,1);
		}
	}
	else
	{
		getip(buf);
		if(ip == NULL)
		{
			printf("Node Not in Configuration\n");
			return -1;
		}
		printf("%s\n",ip);
		return sendcommand(ip,buf,first,last,0);
	}
	return 0;
}

// Manage Execution of Pipeline of Commands Across Nodes
void pipeline(char *buf)
{
	char *pipebuf[MAXCMND];
	for(int i = 0; i < MAXCMND;i++)
           	pipebuf[i]='\0';
        char *arg;
        int first,last;
	int ncmnd = 0;
        arg = strtok(buf,"|");
	while(arg != NULL)
        {
                pipebuf[ncmnd] = arg;
		//printf("%s\n",arg);
		//executecommand(strlen(arg),arg);
                //printf("arg %d %s\n",ncmnd,pipebuf[ncmnd]);
                ncmnd++;
                arg = strtok(NULL,"|");
		while(arg && *arg == ' ')
			arg = arg + 1;
        }
	for(int i = 0 ; i < ncmnd; i++)
	{
		int out;
		if(i == 0)
			first = 1,last = 0;
		else if(i == ncmnd -1)
			first = 0,last = 1;
		else
			first = 0,last = 0;
		if(out = executecommand(strlen(pipebuf[i]),pipebuf[i],first,last))
		{
			if(out == -2){
				printf("Intermediate Node Offline, Data Saved in temp.txt\n");
				break;
			}
			if(out == -1){
				printf("Intermediate Node Not Present, Data Saved in temp.txt\n");
				break;
			}
		}
	}
}

//Cluster Shell Prompt
int main(int argc,char *argv[])
{
	char *path;
	if(argc < 2)
		strcpy(path,"./conf");
	else
		strcpy(path,argv[1]);
	parseconf(path);
	while(1)
	{
		printf("cluster-shell> ");
		fflush(stdout);
		char *buf;
		buf = calloc(50,sizeof(char));
		ip = calloc(13,sizeof(char));
		//fflush(stdin);
		int len = getline(&buf,&buflen,stdin);
		//printf("%s",buf);
		int local = 1;
		if(buf[0] == 'n')
		{
			for(int i = 0 ; i < len ; i++)
				if(buf[i] == '.')
				{
					local = 0;
				}
		}
		if(strcmp(buf,"nodes\n") == 0)
		{
			for(int i = 0 ; i < nodeindex; i++)
			{
				printf("Checking Node: ");
				getip(iplist[i]);
				printf("%s : ",ip);
				fflush(stdout);
				if(checknode(ip) == 1)
					printf("Node Online\n");
				else
					printf("Node Offline\n");
			}
		}
		else if(local == 1)
		{
			system(buf);
		}
		else if(strstr(buf,"|") != NULL)
		{
			printf("Pipeline of Commands: \n");
			pipeline(buf);
		}
		else
		{
			if(executecommand(len,buf,1,1) == -1)
				continue;
		}
	}
	return 0;
}
