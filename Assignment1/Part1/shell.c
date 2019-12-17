/***************************************\ 
*       NP Assignment 1 - Part 1        *
*       Ankur Vineet (2018H1030144P)    *
*       Aman Sharma  (2018H1030137P)    *
\***************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<stddef.h>
#include<mqueue.h>
#include<sys/types.h>
#include<sys/msg.h>
#include<sys/shm.h>

#define MAXARG 25
#define MAX_BUF 4096
#define USEPIPE 1
#define USEMQ 2
#define USESHM 3
#define USEMQBRDCST 4
#define USESHMBRDCST 5
#define SENDTOFILE 6
#define TAKEFROMFILE 7
#define USECOMMA 99
#define APPENDTOFILE 100
#define MSGQ_PATH "./shell.c"
#define SHM_PATH "./shell.c"

const int flags = O_RDWR | O_CREAT ;
size_t buflen = 100;
int background = 0,backcount = 0,dmon = 0;
int backpid[MAXARG];
int pid;
char *arguments[MAXARG];
char saveInp[256];
int _mqbroad = 0;
int _shmbroad = 0;
int _trunc = 1;

struct mbuf{
	long mtype;
	char mtext[MAX_BUF];
};

//Implementation of Only Shared Memory Pipeline -- Finally Not USED
/*
void useSharedMemory(int numArg, char *arg[100][256], int symbolIdx[])
{
	//struct mbuf msg;
	int key,shmid,printbrd = 0,mode;
	char *shmp;
	int i = 0,numSym = 0;
	if((key = ftok (SHM_PATH, 'B')) == -1)perror ("shm ftok");
	if((shmid = shmget (key, MAX_BUF, IPC_CREAT | 0644)) == -1)perror ("shmget");
	char brdbuf[MAX_BUF];
	brdbuf[0] = '\0';
	//printf("Arg: %d\n",numArg);
	for(int i = 0 ; i < numArg; i++)
	{
		mode = findMode(symbolIdx[i], numArg, &numSym, symbolIdx);
		if(mode == USESHMBRDCST)
			_shmbroad = 1;
		int mpipe[2];
		pipe(mpipe);
		int mpid = fork();
		if(mpid == 0){
			int cpipe[2];
			close(mpipe[0]);
			char cbuf[MAX_BUF];
			if(i != 0){	
				bzero(cbuf,MAX_BUF);
				shmp = (char*)shmat(shmid, NULL, 0);
				strcpy(cbuf,shmp);
				int msglen = strlen(cbuf);
				pipe(cpipe);
				write(cpipe[1],cbuf,msglen);
				close(cpipe[1]);
				dup2(cpipe[0],0);
				close(cpipe[0]);
			}
			//read(cpipe[0],buf,20);
			//printf("%s\n",buf);		
			dup2(mpipe[1],1);
			close(mpipe[1]);			
			execvp(arg[i][0], arg[i]);	
		}
		else
		{
			close(mpipe[1]);
			int status;
			wait(&status);
			shmp = (char*) shmat(shmid, NULL, 0);
			char buf[MAX_BUF];
			bzero(buf,MAX_BUF);
			//bzero(msg.mtext,MAX_BUF);
			read(mpipe[0],buf,MAX_BUF);
			if(_shmbroad == 1 && strlen(brdbuf) == 0)
				strcpy(brdbuf,buf);
			close(mpipe[0]);
			//printf("r----%s--r\n",buf);
			if(_shmbroad == 1)
				strcpy(shmp, brdbuf);			
			else
				strcpy(shmp, buf);//, msgLen);
			if(i < numArg -1){
				if(_shmbroad == 1){	
					strcpy(shmp,brdbuf);}
				else{
					strcpy(shmp,buf);}
			}
			if(i == numArg-1 || printbrd == 1)
			{
				printf("%s",buf);
				fflush(stdout);
			}
			if(_shmbroad == 1)
				printbrd = 1;
			
		}
	}	
	
}
*/
//Implementation of Only Message Queue Pipeline -- Finally Not USED
/*
void useMessageQueue(int numArg, char *arg[100][256], int symbolIdx[])
{
	struct mbuf msg;
	int key,msqid,printbrd = 0,mode;
	int i = 0,numSym = 0;
	if((key = ftok (MSGQ_PATH, 'B')) == -1)perror ("ftok");
	if((msqid = msgget (key, IPC_CREAT | 0644)) == -1)perror ("msgget");
	char brdbuf[MAX_BUF];
	brdbuf[0] = '\0';
	//printf("Arg: %d\n",numArg);
	for(int i = 0 ; i < numArg; i++)
	{
		mode = findMode(symbolIdx[i], numArg, &numSym, symbolIdx);
		if(mode == USEMQBRDCST)
			_mqbroad = 1;
		int mpipe[2];
		pipe(mpipe);
		int mpid = fork();
		if(mpid == 0)
		{
			int cpipe[2];
			close(mpipe[0]);
			char cbuf[MAX_BUF];
			if(i != 0)
			{	
				bzero(cbuf,MAX_BUF);
				int msglen = msgrcv(msqid , &(msg.mtype), sizeof(msg), 420, 0);
				cbuf[msglen] = '\0';
				//if(msgctl (msqid, IPC_RMID, NULL) == -1)perror ("msgctl");
				strcpy(cbuf, msg.mtext);
				//printf("%d\n",msglen);
				//printf("--i %d ch: %s--\n",i,cbuf);
				pipe(cpipe);
				write(cpipe[1],cbuf,msglen);
				close(cpipe[1]);
				dup2(cpipe[0],0);
				close(cpipe[0]);
			}
			//read(cpipe[0],buf,20);
			//printf("%s\n",buf);		
			dup2(mpipe[1],1);
			close(mpipe[1]);			
			execvp(arg[i][0], arg[i]);
			
		}
		else
		{
			close(mpipe[1]);
			int status;
			wait(&status);
			char buf[MAX_BUF];
			bzero(buf,MAX_BUF);
			bzero(msg.mtext,MAX_BUF);
			read(mpipe[0],buf,MAX_BUF);
			if(_mqbroad == 1 && strlen(brdbuf) == 0)
				strcpy(brdbuf,buf);
			close(mpipe[0]);
			//printf("r----%s--r\n",buf);
			msg.mtype = 420;
			//fprintf(stderr, "Message in MSG QUEUE : %ld\n", msgLen);
			if(_mqbroad == 1)
				strcpy(msg.mtext, brdbuf);			
			else
				strcpy(msg.mtext, buf);//, msgLen);
			if(i < numArg -1)
			{
				if(_mqbroad == 1)
				{	
					if((status = msgsnd(msqid, &(msg.mtype), strlen(brdbuf), 0 )) == -1)perror("msg snd");
				}
				else
				{
					if((status = msgsnd(msqid, &(msg.mtype), strlen(buf), 0 )) == -1)perror("msg snd");
				}
			}
			if(i == numArg-1 || printbrd == 1)
			{
				printf("%s",buf);
				fflush(stdout);
			}
			if(_mqbroad == 1)
				printbrd = 1;
			
		}
	}	
	
}
*/


//Handle Processes Killed or Stopped while Running in Background
void handlebackground(int sig)
{
	int status;
	for(int i = 0 ; i < backcount; i++)
	{
		int rpid = waitpid(backpid[i],&status,WNOHANG);
		if(rpid == backpid[i])
		{
			if(WIFSTOPPED(status))
			{
				printf("Process Group with PGID %d stopped %d.\n",pid,status);
				backpid[backcount] = pid;
				backcount++;
			}
			else if(WIFEXITED(status))
			{
				printf("Process Group with PGID %d exited with status %d.\n",pid,status);
				while(i < backcount-1)
				{
					backpid[i] = backpid[i+1];
					i++;
				}
				backcount--;
			}
			break;
		}
	}
}

char *readfile(char *filename){

	char *buffer = 0;
	long length;
	FILE *f = fopen(filename, "rb");
	if(f){
		fseek (f, 0, SEEK_END);
		length = ftell (f);
		fseek (f, 0, SEEK_SET);
		buffer = malloc (length);
		if (buffer){
	    	fread (buffer, 1, length, f);
	  	}
	  	fclose (f);
	}
	return buffer;
}

// Name the mode corresponding to the special symbol
int findMode(int idx, int numArg, int *numSym, int symbolIdx[]){

	switch(saveInp[idx]){
		case '|' : // Use Pipe
					return USEPIPE;
		break;

		case '<' : // Use Redirection
					return TAKEFROMFILE;
		break;

		case '>' : // Use Redirection 2
					if(saveInp[idx - 1] == '>'){
						//printf("CHAR ---> : %c\n", saveInp[idx-1]);
						return APPENDTOFILE;
					}
					return SENDTOFILE;
		break;

		case '#' : //Use Message Queue Ahead
					//printf("Char :%c\n", saveInp[idx-1]);
					if(saveInp[idx - 1] == '#'){
						*numSym = 2;
						return USEMQBRDCST;
					}
					return USEMQ;
		break;

		case 'S' : // Use Shared Memory
					if(saveInp[idx - 1] == 'S'){
						*numSym = 2;
						return USESHMBRDCST;
					}
					return USESHM;
		break;
		case ',' : 	return USECOMMA;
		break;
	}
	return -1;
}

//Execute n-1 child processes from the n commands passed concatenated using special symbols -- Supports Generic Pipeline
int executeChild(int readfd, int writefd, char *arg[100][256], int idx, int prevMode, int mode)
{
	pid_t pid;
	int status;
	struct mbuf msg;
	int shmid;
	char *shmp;
	ssize_t msgLen;
	char auxbuf1[MAX_BUF];
	int type = 420, mflag = 0;

	if((pid = fork()) == 0)
	{
		printf("Process Id: %d\n",getpid());
		if(prevMode == USEPIPE){
			if(readfd != STDIN_FILENO){
				dup2(readfd, STDIN_FILENO);
				close(readfd);
			}
		}
		else if(prevMode == SENDTOFILE || prevMode == APPENDTOFILE)
		{
			int dnull = open("/dev/null",O_RDONLY);
			dup2(dnull,STDIN_FILENO);
			close(dnull);
		}
		else if(prevMode != 0){
			int fid, msqid;
			key_t key;

			memset(&msg, 0, sizeof(struct mbuf));
			memset(auxbuf1, 0, MAX_BUF);
			if(prevMode == USEMQ || prevMode == USEMQBRDCST || _mqbroad){
				if((key = ftok (MSGQ_PATH, 'B')) == -1)perror ("ftok");
				if((msqid = msgget (key, IPC_CREAT | 0644)) == -1)perror ("msgget");
				msgLen = msgrcv(msqid , &(msg.mtype), sizeof msg, 420, 0);
				strcpy(auxbuf1, msg.mtext);
				fprintf(stderr, "Received Message in Queue : type=%ld; length=%ld \n", msg.mtype, (long)msgLen);
			}
			else if(prevMode == USESHM || prevMode == USESHMBRDCST || _shmbroad){
				//printf("IM HERE %d\n", MAX_BUF);
				if((key = ftok (SHM_PATH, 'B')) == -1)perror ("shm ftok");
				if((shmid = shmget (key, MAX_BUF, IPC_CREAT | 0644)) == -1)perror ("shmget");
				shmp = (char*)shmat(shmid, NULL, 0);
				if(shmp == NULL) perror("shmat");
				strcpy(auxbuf1, shmp);
				printf("Read from SHARED MEMORY (SIZE): %ld\n",strlen(auxbuf1));
			}
			msgLen = strlen(auxbuf1);
			int pipeInter[2];
			//if(msgLen == 0) perror("NO DATA \n");
			if(msgLen > 0){
				/*//lseek(STDIN_FILENO, 0, 0);
				fid = open("temp.txt", flags | O_TRUNC, 0666);
				write(fid, auxbuf1, msgLen);
				dup2(fid, STDIN_FILENO);
				lseek(fid, 0, 0);
				*/
				//printf("CMD :%s\n", arg[idx][0]);
				//	if(msgctl (readfd, IPC_RMID, NULL) == -1)perror ("msgctl");
					pipe(pipeInter);
					write(pipeInter[1], auxbuf1, msgLen);
					close(pipeInter[1]);
					
					if(pipeInter[0] != STDIN_FILENO){
						dup2(pipeInter[0], STDIN_FILENO);
						close(pipeInter[0]);
					}
				}
		}
		if(prevMode != USEMQBRDCST && prevMode != USESHMBRDCST)
			if(writefd != STDOUT_FILENO){
					dup2(writefd, STDOUT_FILENO);
					close(writefd);
			}
		execvp(arg[idx][0], arg[idx]);
		perror("Child All: ");
	}
}
//Control Single Job of n Commands and Returns when all the commands execute -- Implements Generic Pipeline
int prepareForExec(int numArg, char *arg[100][256], int symbolIdx[])
{
	pid_t pid;
	ssize_t msgLen, maxBytes;
	struct mbuf msg;
	char *shmp;
	char auxbuf1[MAX_BUF];
	char auxbuf2[MAX_BUF];
	char filen[20];
	key_t key;
	memset(auxbuf1, 0, sizeof auxbuf1);
	memset(auxbuf2, 0, sizeof auxbuf2);
	memset(filen, 0, sizeof filen);
	int i = 0, mode = 0, type = 420;
	int numSym = 0, mflags = 0, msqid, shmid;
	int pipefd[2], readfd = 0, filefd[2], status = 0;
	int prevmode = 0;
	char intermediateSave[MAX_BUF];
	for(i = 0; i < numArg - 1; i++){

		mode = findMode(symbolIdx[i], numArg, &numSym, symbolIdx);

		if(mode == USEMQBRDCST) _mqbroad = 1;
		else if(mode == USESHMBRDCST) _shmbroad = 1;
		else if(mode == APPENDTOFILE) _trunc = 0;
		if(mode < 0) return -1;
		sprintf(filen, "%d.txt", i);
		//Uncomment To See the Working of Unused Functions
		/*if(mode == USEMQ || mode == USEMQBRDCST)
		{
			useMessageQueue(numArg,arg,symbolIdx);
			return 0;
		}
		if(mode == USESHM || mode == USESHMBRDCST)
		{
			useSharedMemory(numArg,arg,symbolIdx);
			return 0;
		}*/
		if(mode == USEPIPE)
		{
			pipe(pipefd);
			//execute command
			printf("Pipe FD for Read End: %d Write End %d\n",pipefd[0],pipefd[1]);
			//printf("Reading from FD %d , Writing To FD %d\n",readfd,pipefd[1]);
			status = executeChild(readfd, pipefd[1], arg, i, prevmode, mode);
			close(pipefd[1]);
			int status;
			wait(&status);
			//lseek(filefd, 0, 0);
			readfd = pipefd[0];
		}
		else if(mode != TAKEFROMFILE) 
		{
			//strcpy(filen, "other.txt");
			//filefd = open(filen, flags | O_TRUNC, 0666);
			pipe(filefd);
			if(mode == USEMQBRDCST || mode == USEMQ){
				if((key = ftok (MSGQ_PATH, 'B')) == -1)perror ("ftok");
				if((msqid = msgget (key, IPC_CREAT | 0644)) == -1)perror ("msgget");
			}
			if(mode == USESHM || mode == USESHMBRDCST){
				if((key = ftok (SHM_PATH, 'B')) == -1)perror ("shm ftok");
				if((shmid = shmget (key, MAX_BUF, IPC_CREAT | 0644)) == -1)perror ("shmget");
				shmp = (char*) shmat(shmid, NULL, 0);
				if(shmp == NULL) perror("shmat");
			}
			if(mode == SENDTOFILE || mode == APPENDTOFILE){
				int ffd;
				char filename[20];// =
				strcpy(filename, arg[i+1][0]);
				printf("SEND TO FILE :%s From FD %d\n", arg[i+1][0], filefd[0]);
				int fg = flags | O_TRUNC;
				if(mode == APPENDTOFILE){
					fg = flags | O_APPEND;
				}
				ffd = open(filename, fg, 0666);
				filefd[1] = ffd; // FD REMAPPING FOR FD
			}
			status = executeChild(readfd, filefd[1], arg, i, prevmode, mode);
			close(filefd[1]);
			int stat;
			wait(&stat);
			if(mode == SENDTOFILE || mode == APPENDTOFILE)
			{
					printf("Output Redirected to File FD: %d\n",filefd[1]);
					i++;
			}
			//lseek(filefd, 0, 0);
			//perror("1");
			//if(mode != USECOMMA)
			readfd = filefd[0];
		}
		else 
		{
			int ffd, tfd[2];
			pid_t pid;
			pipe(tfd);
			if((pid = fork()) == 0)
			{
				char filename[20];
				strcpy(filename, arg[i+1][0]);
				close(tfd[0]);
				int fgs = flags;
				if((ffd = open(filename, O_RDONLY , 0666)) < 0 ){
					perror("FILE NOT THERE\n");
					exit(0);
				}
				printf("Input Redirected From File FD %d\n",ffd);
				printf("Output Given To FD %d\n",tfd[1]);
				//readfd = tfd;
				// THIS PART HELPS THE INPUT REDRECTION TO CONTINUE THE CHAIN, Remodelled using PIPES
				//if((tfd = open("IMP.txt", flags | O_TRUNC, 0666)) < 0 ){
				//	perror("IMP FILE NOT THERE\n");
				//}
				dup2(ffd, STDIN_FILENO);
				close(ffd);
				if( i != numArg - 2){
					dup2(tfd[1], STDOUT_FILENO);
					close(tfd[1]);
				}
				execvp(arg[i][0], arg[i]);
				perror("Input Redirection: ");
			}
			int stat;
			wait(&stat);
			close(tfd[1]);
			dup2(tfd[0], readfd);
			close(tfd[0]);
			mode = findMode(symbolIdx[i+1], numArg, &numSym, symbolIdx);
			//if((tfd = open("IMP.txt", flags, 0666)) < 0 ){
			//		perror("IMP FILE NOT THERE\n");
			//}
			
			i++;
			//strcpy(filen, "IMP.txt");
		}
		prevmode = mode;
		if(prevmode != USEPIPE && prevmode != SENDTOFILE && prevmode != APPENDTOFILE && prevmode !=TAKEFROMFILE)
		{
			// SENDING THE MESSAGE THAT HAS BEEN WRITTEN BY PREV CMD TO MSG QUEUE
			char filecont[MAX_BUF];
			memset(filecont, 0, sizeof(filecont));

			//memset(filecont, 0, sizeof(filecont));
			int readbytes = read(readfd, filecont, MAX_BUF);
			memset(&msg, 0, sizeof msg);
			if(readbytes < 0) perror("read ");
			if(prevmode == USEMQBRDCST ){
				strcpy(intermediateSave, filecont);
			}
			if(prevmode == USESHM || prevmode == USESHMBRDCST){
				fprintf(stderr, "Message in SHARED MEM (SIZE): %ld\n", strlen(filecont));
				strcpy(shmp, filecont);
				continue;
			}
			if(prevmode == USEMQBRDCST || prevmode == USEMQ || prevmode == USECOMMA)
			{
				if((key = ftok (MSGQ_PATH, 'B')) == -1)perror ("ftok");
				if((msqid = msgget (key, IPC_CREAT | 0644)) == -1)perror ("msgget");
				if(prevmode == USECOMMA){
					memset(filecont, 0, sizeof(filecont));
					strcpy(filecont, intermediateSave);	
				}
				msgLen = strlen(filecont);
				msg.mtype = 420;
				fprintf(stderr, "Message in MSG QUEUE : %ld\n", msgLen);
				strcpy(msg.mtext, filecont);//, msgLen);
				if((status = msgsnd(msqid, &(msg.mtype), msgLen, 0 )) == -1)perror("msg snd");
			}
		}
		if(prevmode == SENDTOFILE || prevmode == APPENDTOFILE)
		{
			int dnull = open("/dev/null",O_RDONLY);
			readfd = dnull;
		}
		fprintf(stderr, "--------------------------------------\n");
		///printf(" MODE :%d\n", prevmode);
		//lseek(filefd, 0, 0);

	}
	// Loop for n-1 Command Ends
	printf("Process Id: %d\n",getpid());
	if(prevmode == SENDTOFILE || prevmode == APPENDTOFILE)
	{
		int dnull = open("/dev/null",O_RDONLY);
		dup2(dnull,STDIN_FILENO);
		close(dnull);
		if(i == numArg)
			exit(0);
		execvp(arg[i][0], arg[i]);
		perror("Single Program: ");
	}
	if(prevmode == 0 || prevmode == USEPIPE)
	{
		if(readfd != STDIN_FILENO){
			dup2(readfd, STDIN_FILENO);
			close(readfd);
		}
		if(dmon == 1)
		{
			//daemon(0,0);
			int dpid = fork();
			if(dpid == 0)
			{
				setsid();
				//chdir("/");
				close(STDIN_FILENO);
				int fd = open("/dev/null", O_RDWR);
				if (fd != STDIN_FILENO) /* 'fd' should be 0 */
					return -1;
				if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
					return -1;
				if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
					return -1;
				execvp(arg[i][0], arg[i]);
				perror("Daemon: ");
			}
			else
				exit(0);
		}
		
		else{
			printf("Write to stdout\n");
			execvp(arg[i][0], arg[i]);
			perror("Single Program: ");
		}
		return 0;
	}
	else if(prevmode != USEPIPE)
	{
		int fid, ffid[2];
		/*if(prevmode == USEMQBRDCST || prevmode == USESHMBRDCST || prevmode == USECOMMA){
			char *file = readfile(filen);
			fputs(file, stdout);
		}*/
		memset(&msg, 0, sizeof(struct mbuf));
		memset(&shmp, 0, MAX_BUF);
		memset(auxbuf1, 0, MAX_BUF);

		if(prevmode == USEMQ || prevmode == USEMQBRDCST || _mqbroad){
			msgLen = msgrcv(msqid , &(msg.mtype), sizeof msg, 420, 0);
			if(msgLen == -1) perror("Msg Rcv 2");
			fprintf(stderr, "Received Message in Queue : type=%ld; length=%ld \n", msg.mtype, (long)msgLen);
			strcpy(auxbuf1, msg.mtext);
		}
		else if(prevmode == USESHM || prevmode == USESHMBRDCST || _shmbroad) {
			if((key = ftok (SHM_PATH, 'B')) == -1)perror ("shm ftok");
			if((shmid = shmget (key, MAX_BUF, IPC_CREAT | 0644)) == -1)perror ("shmget");
			shmp = (char*)shmat(shmid, NULL, 0);
			if(shmp == NULL) perror("shmat");
			strcpy(auxbuf1, shmp);
			printf("Read from SHARED MEMORY (SIZE): %ld\n",strlen(auxbuf1));
		}
		fprintf(stderr, "--------------------------------------\n");

		msgLen = strlen(auxbuf1);
		if(msgLen > 0)
		{
				//lseek(STDIN_FILENO, 0, 0);
				/*fid = open("temp.txt", flags | O_TRUNC, 0666);
				write(fid, auxbuf1, msgLen);
				dup2(fid, STDIN_FILENO);
				lseek(fid, 0, 0);*/
				pipe(ffid);
				write(ffid[1], auxbuf1, msgLen);
				close(ffid[1]);
				dup2(ffid[0], STDIN_FILENO);
				close(ffid[0]);
				if(prevmode == USEMQ || prevmode == USEMQBRDCST || prevmode == USECOMMA)
				if(msgctl (msqid, IPC_RMID, NULL) == -1)perror ("msgctl");
				execvp(arg[i][0], arg[i]);
				perror("Shared Program: ");
				return 0;
		}

	}
}

//Parse the input command and extract the special operators in it
void parseCommand(char input[], char *arg[100][256], int *numArg, int symbolIdx[]){

	char *str[256];
	char symbols[] = "<>|#S,";
	int i = 0, j = 0;
	strcpy(saveInp, input);
	str[i] = strtok(input, symbols);
	int len = 0;
	while(str[i] != NULL){
		i++;
		str[i] = strtok(NULL, symbols);
		if(str[i]){
			len = str[i] - input;
			symbolIdx[j] = len - 1;
		}
		j++;
	}
	*numArg = i;
	char  spacermv[] = " \n";
	int l = 0;
	int k;
	for(k = 0; k < i; k++){
		l = 0;
		arg[k][l] = strtok(str[k], spacermv);
		while(arg[k][l] != NULL){
			l++;
			arg[k][l] = strtok(NULL, spacermv);
		}
		arg[k][l] = NULL;
	}
	if(k > 0 && l > 0 && arg[k-1][l-1][0] == '&')
	{
		background = 1;
		arg[k-1][l-1] = NULL;
	}

}

//Get Back the Control from the Executing Job to Shell
void getcontrol(int pid)
{
	int status;
	int rpid = waitpid(pid,&status,WUNTRACED);
	int fd = open("/dev/tty",O_RDWR);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	tcsetpgrp(fd, getpid());
	if(WIFEXITED(status))
	{
		printf("Process Group with PGID %d exited with status %d.\n",pid,status);
	}
	else if(WIFSTOPPED(status)){
		printf("Process Group with PGID %d stopped.\n",pid);
		backpid[backcount] = pid;
		backcount++;
	}
}

//Resume Background Job in Foreground
void resumeinforeground()
{
	if(backcount <= 0){
		printf("No Job to Continue\n");
		backcount = 0;
	}
	else
	{
		int resumepid = backpid[0];
		//if(backcount <= 1)
			//backpid[0] = 0;
		for(int i = 0 ; i < backcount-1; i++)
		{
			backpid[i] = backpid[i+1];
			//printf("bp %d\n",backpid[i]);
		}
		backcount--;
		printf("Resuming Process Group with PGID in Foreground: %d\n",resumepid);
		int fd = open("/dev/tty",O_RDWR);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		tcsetpgrp(fd, resumepid);
		kill(resumepid,SIGCONT);
		getcontrol(resumepid);
	}
}
//Resume Job in Background
void resumeinbackground()
{
	if(backcount == 0)
		printf("No Job to Continue\n");
	else
	{
		int resumepid = backpid[0];
		printf("Resuming Process Group with PGID in Background: %d\n",resumepid);
		kill(resumepid,SIGCONT);
	}
}
//Job Controller - Handling Multiple Jobs
void executeprogram(int numArg, char *arg[100][256], int symbolIdx[])
{
	//signal(SIGTTOU,outputhandle);
	int status;
	pid = fork();
    if(pid == 0)
    {
    	//printf("Child Process\n");
		int fd = open("/dev/tty",O_RDWR);
	  	if (setpgid(getpid(), 0) != 0)
	       	perror("setpgid() error");
	   	else if(background == 0)
		{
			signal(SIGTTOU, SIG_IGN);
			signal(SIGTTIN, SIG_IGN);
	       	if (tcsetpgrp(fd, getpid()) != 0)
	       		perror("tcsetpgrp() error");
	       	else
	       		printf("New foreground process group id for terminal: %d\n",(int) getpid());
	   	}
		prepareForExec(numArg, arg, symbolIdx);
		//execvp(arguments[0],arguments);
		//perror("execve");
       	exit(EXIT_SUCCESS);
    }
    else
    {
		signal(SIGCHLD,handlebackground);
		if(background == 0)
		{
			//pause();
			//waitpid(pid,&status,WUNTRACED);
			getcontrol(pid);
		}
		else if(background == 1 && dmon == 0)
		{
			printf("Running as Background Process with PGID: %d\n",pid);
			backpid[backcount] = pid;
			backcount++;
			background = 0;
		}
		else if(background == 1 && dmon == 1)
		{
			waitpid(pid,&status,0);
			background = 0;
			dmon = 0;
		}
	}
}
//Custom Shell Prompt
int main()
{
	printf("Custom Shell:\n");
	while(1)
	{
		char *arg[100][256];
		int numArg = 0;
		int symbolIdx[20];
		char *buf;
		printf("custom-shell> ");
		background = 0;
		fflush(stdout);
		int len = getline(&buf,&buflen,stdin);

		if(buf[len-1] == '\n')
			buf[len-1] = ' ';
		if(strcmp(buf,"fg ") == 0)
			resumeinforeground();
		else if(strcmp(buf,"bg ") == 0)
			resumeinbackground();
		else if(strcmp(buf," ") == 0)
			continue;
		else
		{
			if(strstr(buf,"daemonize"))
			{
				buf = buf + 10;
				dmon = 1;
				background = 1;
			}
			parseCommand(buf, arg, &numArg, symbolIdx);
			//for(int i = 0; i < numArg; i++){
			//int j = 0;
			//while(arg[i][j])
			//{
			//	printf("cmd %d : %s\n",i,  arg[i][j] );
			//	j++;
			//}
			//printf("sym %d : %c\n",i, saveInp[symbolIdx[i]]);
			//}
			//parsetext(buf);
			executeprogram(numArg, arg, symbolIdx);
		}
	}
	return 0;
}
