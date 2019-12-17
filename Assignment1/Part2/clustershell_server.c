/***************************************\ 
*	    NP Assignment 1 - Part 2        *
*       Ankur Vineet (2018H1030144P)	*
*       Aman Sharma  (2018H1030137P)	*
\***************************************/

#include<stdio.h>
#include<pwd.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<string.h>
#define MAXARG 25
#define CMDLENGTH 50

char *arguments[MAXARG];
void parsetext(char *buf)
{
        for(int i = 0; i < MAXARG;i++)
                arguments[i]='\0';
	int len = strlen(buf);
	if(buf[len-1] == '\n')
		buf[len-1] = ' ';
        char *arg;
        int i = 0;
        arg = strtok(buf," \n");
        while(arg != NULL)
        {

                arguments[i] = arg;
                //printf("arg %d %s\n",i,arguments[i]);
                i++;
                arg = strtok(NULL," ");

        }
}

int main()
{
	int listenfd,connfd;
	struct sockaddr_in serveraddr;
  	int def = 0;
	if((listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
		perror("Socket: ");
	serveraddr.sin_port = 8000;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if((bind(listenfd, (struct sockaddr *) & serveraddr , sizeof(serveraddr))) < 0)
		perror("Bind: ");
	listen(listenfd,5);
	while(1)
	{
		fflush(stdin);
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
		char recvchar[CMDLENGTH];
		char homedir[CMDLENGTH];
		for(int i = 0 ; i < CMDLENGTH; i++){
			recvchar[i] = 0;
			homedir[i] = 0;
		}
		int n = read(connfd,homedir,CMDLENGTH);
		homedir[n] = '\0';
    if(def == 0)
    {
      if(chdir(homedir)<0);
      else
	      def = 1;
    }
		//printf("%s\n",homedir);
		n = read(connfd,recvchar,CMDLENGTH);
       		recvchar[n] = '\0';
		dup2(connfd,1);
        	dup2(connfd,2);
		//printf("%d\n",n);
		dup2(connfd,0);
		if(recvchar[0] == 'c' && recvchar[1] == 'd')
		{
			//write(1,recvchar + 3,16);
			//printf("Executing cd: %s\n",strtok(recvchar + 3," \n"));
			//char s[100];
    			// printing current working directory
    			//write(1, getcwd(s, 100),100);
    			// using the command
    			//chdir("..");
    			// printing current working directory
    			//write(1, getcwd(s, 100),100);
			if(chdir(strtok(recvchar + 3," \n")) < 0)
				perror("chdir:");
		}
		else if(n > 0)
		{
			parsetext(recvchar);
			int pid = fork();
			if(pid == 0)
			{
				execvp(arguments[0],arguments);
				perror("execvp");
				exit(0);
			}
			else
			{
				int status;
				wait(&status);
			}
			//system(recvchar);
		}
		close(connfd);
		close(0);
		close(1);
		close(2);
	}
	return 0;
}
