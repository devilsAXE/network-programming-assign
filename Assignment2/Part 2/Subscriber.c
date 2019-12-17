/********************************************************************
                File: -  SUBSCRIBERv1.0
            Features: 1. Create a topic
                      2. Send a message tagged under a topic
                      3. Take a file and send as a series of messages

********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include "pubsub.h"



#define clrscr() printf("\033[H\033[J")

#define MAX_MSG_DATA 512
#define PORT 12001
#define MAXTOPICS 50
#define CREATE 9
#define EXPIRED 10
#define INFO 11
#define BACKLOG 10


int send_message(int, char*, int , long);
void show_menu(int);
void trim(char*);
int rec_message(int);
int log_operation(char*, int, int, int);
int update_broker_offsets_for_topic(char*,struct requestmsg*, int);



char topics[MAXTOPICS][20];
long offsets[50], myPort;
int topic_idx = -1;
char curr_topic[20];
long myId;
char *myIP;
long mach_offset[100];

static void zombie_cleaner(int sig){
	while(waitpid(-1, NULL, 0) > 0){
	  fprintf(stderr, "[STATUS]: Client Disconnected ----- \n");
	  continue;
	}
}
int main(int argc, char *argv[]){

  char ip_brokr[20];
  myId = getpid();
  int sock_fd;
  char dirname[10];
  sprintf(dirname, "%ld", myId); // Folder to keep msgs under topics
  if(mkdir(dirname, 0777) == -1){
    perror("mkdir");
    exit(EXIT_FAILURE);
  }
  int custom_port, listen_port;
  struct sockaddr_in servaddr;
  if(argc > 1){
    strcpy(ip_brokr, argv[1]);
    custom_port = atoi(argv[2]);
    listen_port = atoi(argv[3]); 
  }
  else{
    fprintf(stderr, "[WARNING]: Usage <ip> <port> \n");
    exit(1);
  }

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd < 0) err_sys("socket");
  
  memset(topics, 0, sizeof(topics));
  memset(&servaddr, 0, sizeof(servaddr));
  memset(offsets, 0, sizeof(offsets));
  for(int i = 0; i < MAXTOPICS; i++){
    offsets[i] = 0;
    strcpy(topics[i], "");
  }
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(custom_port);
  servaddr.sin_addr.s_addr = inet_addr(ip_brokr);


  if(connect(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  err_sys("connect");


  struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
  
  int listenfd, maxfd, optval = 1;
  struct sockaddr_in servaddr2, clientaddr2, my_addr;
  socklen_t addrlen;
  fd_set readfds, fds;
  
  bzero(&my_addr, sizeof(my_addr));
	int len = sizeof(my_addr);
	getsockname(sock_fd, (struct sockaddr *) &my_addr, &len);
	inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
	
  myPort = listen_port;
  myIP = inet_ntoa( my_addr.sin_addr);
 
  pid_t chpid = fork();
  if(chpid == 0){
    //Start listening for msgs directly from Brokers
    close(sock_fd);
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      err_sys("socket");
    
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    memset(&servaddr2, 0, sizeof(servaddr2));
    servaddr2.sin_port = htons(listen_port);
    servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if((bind(listenfd, (struct sockaddr*)&servaddr2, sizeof(servaddr2))) < 0 ){
      err_sys("bind");
    }
    if(listen(listenfd, BACKLOG) < 0){
      err_sys("listen");
    }
    fprintf(stderr, "Listening at Port : [%d]\n", listen_port);
    maxfd = listenfd;
    FD_ZERO(&readfds);
    int fd_new, nready;
    FD_SET(listenfd, &readfds);
    for(;;){
      readfds = fds;
      if(select(maxfd + 1, &readfds, NULL, NULL, &tv) < 0){
        err_sys("Select");
      }
      for(int fd = 0; fd <= maxfd; fd++){
        if(FD_ISSET(fd, &readfds)){
          if(fd == listenfd){
            addrlen = sizeof clientaddr2;
            fd_new = accept(listenfd, (struct sockaddr*)&clientaddr2, &addrlen);
            if(fd_new < 0) err_sys("accept");
            FD_SET(fd_new, &fds);
            maxfd = MAX(maxfd, fd_new);
            //fprintf(stderr, "[STATUS]: client = %s connected at Port = %d\n", inet_ntoa(clientaddr2.sin_addr), listen_port);
        }
          else{              
              if(rec_message(fd) < 0){
                FD_CLR(fd, &fds);
                close(fd);
              }
          }
        }
      }
    }

  }else if(chpid > 0){
    show_menu(sock_fd);
    wait(NULL);
  }
  exit(EXIT_SUCCESS);
}

void err_sys(char* x){
    perror(x);
    exit(1);
}

void show_menu(int sock_fd){
  fprintf(stderr,"[MENU]: Enter one of the options at any time \n");
  char inbuf [512];
  while(1){
    fprintf(stderr,"[MENU]: [1.] Subscribe to a topic [2.] Retrieve next message [3.] Retrieve continuously all messages \n");
    int options;
    int len;
    long offset;
    char topic_name[20];
    if (fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
        perror ("fgets");
    sscanf (inbuf, "%d", &options);
    switch (options) {
      case 1:printf("[MENU]: Enter topic name (max 20 chars): \n");
              if (fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
                perror ("fgets");
              len = strlen(inbuf);
              if(inbuf [len - 1] == '\n')
                inbuf [len - 1] = '\0';
              strcpy(curr_topic, inbuf);
              trim(curr_topic);
              fprintf(stderr,"[STATUS]:Subscribed to '%s'\n", curr_topic);
              break;

      case 2: //clrscr();
              if(curr_topic == NULL){
                fprintf(stderr, "[WARNING]: **** Subscribe to a topic first ****\n");
                break;
              }
              fprintf(stderr, "[STATUS]: Retrieving message of topic '%s'\n", curr_topic);
              send_message(sock_fd, curr_topic, 1, 1); // request messages for given topic starting from offset
              if(rec_message(sock_fd) < 0);
              break;

      case 3: 
              if(curr_topic == NULL){
                fprintf(stderr, "[WARNING]:**** Subscribe to a topic first ****\n");
                break;
              }
              fprintf(stderr, "[STATUS]: Retrieving message of topic '%s'\n", curr_topic);
              send_message(sock_fd, curr_topic, 1, -1); // request messages for given topic starting from offset
              if(rec_message(sock_fd) < 0);
              break;

      default: clrscr();
               printf("[WARNING]: Illegal option, try again\n");
               break;
    }
  }
}

int send_message(int sock_fd, char *curr_topic, int nmsg, long off){
  struct requestmsg rqst;
  int isAll = 1;
  size_t msg_len = sizeof (struct requestmsg);
  strcpy(rqst.topic, curr_topic);
  strcpy(rqst.sub_ip, myIP);
  rqst.sub_port = myPort;
  rqst.no_of_msg = nmsg;
  if(off < 0) isAll = -1;
  update_broker_offsets_for_topic(curr_topic, &rqst, isAll); 
  fprintf(stderr, "[REQUEST]: topic = '%s', type = %s\n", curr_topic, (isAll > 0) ? "NXTMSG": "ALLMSG");
  if (send (sock_fd, &rqst, msg_len, 0) == -1)
    perror ("send");
  return 0;
}

int rec_message(int sock_fd){
  struct message response;
  char *recv_topic, *recv_content;
  long mac_id, offset;
  char logfile[20];
  for(int i = 0; i < 100; i++) mach_offset[i] = 0;
  memset(&response, '\0', sizeof(struct message));
  int tot_msgs = 0;
  while(recv(sock_fd, &response, sizeof(struct message), 0 ) > 0){
    mac_id = response.machine_id;
    offset = response.offset;
    recv_topic = (char *)malloc(strlen(response.topic) + 1);
    recv_content = (char *)malloc(strlen(response.content) + 1);
    strcpy(recv_topic, response.topic);
    strcpy(recv_content, response.content);
    trim(recv_topic);trim(recv_content);
    fprintf(stderr,"[RESPONSE]: from = [%d], topic = '%s', offset = [%d]\n", abs(mac_id), recv_topic, abs(offset));
    sprintf(logfile, "%ld/%s/%s.log", myId, recv_topic, recv_topic); 
    if(strstr(recv_topic, "NOTFOUND") != NULL){
      return -1;
    }
    char filename[30];
    sprintf(filename, "%ld/%s",myId,recv_topic);
    if(mkdir(filename, 0777) == -1){ //Created a Topic Directory
    }
    else{
        // Creating log file (once)
        FILE *fptr;
        if((fptr = fopen(logfile, "w+")) == NULL){
          perror("fopen");
          exit(EXIT_FAILURE);
        }
        //Setting up all machine offset as zero
        fseek(fptr, 0, SEEK_SET);
        for(int i = 1; i <= 100; i++){
          fprintf(fptr, "machine_id: %d       \t last_file: %d       \n", i, 1);
        }
        fclose(fptr);
    }
    FILE* fptr;
    sprintf(filename, "%ld/%s/%d_%ld.dat",myId, recv_topic, abs(mac_id), offset); //writing message to Topic directory
    if((fptr = fopen(filename, "w")) == NULL){
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    fputs(recv_content, fptr);
    fclose(fptr);
    mach_offset[(abs(mac_id))] = abs(offset)+1;
    tot_msgs++;
    if(mac_id < 0 || offset < 0){
      mac_id = abs(mac_id);
      break;
    }
  }
  if(strstr(recv_topic, "NOTFOUND")!= NULL){
    return -1;
  }
  /********************UPDATING LOG FILE**************************************/
    char line[1024];
    FILE *logupdate;
    if((logupdate = fopen(logfile, "r+")) == NULL){
      perror("Logging State:");
      return -1;
    }
    int lin = 1, pos;
    //fprintf(stderr, "Machine ID [%ld] & Offset [%ld]\n", mac_id,  mach_offset[mac_id]);
    while(fgets(line, sizeof line, logupdate) != NULL){
      pos = ftell(logupdate);
      if(lin == mac_id){
        fseek(logupdate, pos - strlen(line), SEEK_SET);
        fprintf(logupdate, "machine_id: %ld       \t last_file: %ld", mac_id, mach_offset[mac_id]);
      }
      lin++;
    }
    fclose(logupdate);
  /****************************************************************************/
  fprintf(stderr,"[STATUS]: total messages = %d of topic = '%s' downloaded from broker = %d\n", tot_msgs, recv_topic, abs(mac_id));
  fprintf(stderr, "[STATUS]: Closing Connection \n\n");
  return -1;

}

int log_operation(char* dir, int ops, int mac_id, int last_file){
  char line[100], needle[20];
  size_t len = 0;
  ssize_t read;
  char filename[20];
  int pos  = 0;
  sprintf(needle, "machine_id: %d", mac_id);
  sprintf(filename, "%ld/%s/%s.log",myId, dir, dir);
 // fprintf(stderr, "Checking %s", needle);
  int var, offset;  
  FILE *fptr = fopen(filename, "r+");
  if(fptr == NULL){
    perror("log fopen");
    return -1;
  }
  //long first_file, last_file;
  fseek(fptr, 0, SEEK_SET);
  if(ops == 0){
    while(fgets(line, sizeof line, fptr) != NULL) {
      if(strstr(line, needle) != NULL){
       sscanf(line,"%*s %d\t%*s %d",&var, &offset);
        fclose(fptr);
        return offset;
      }
    }
    return 0;
  }
  else if(ops == 1){
    int lin = 1, off;
    while(fgets(line, sizeof line, fptr) != NULL) {
      pos = ftell(fptr);
      if(lin == mac_id){ 
        // fprintf(stderr, "%s", line);
        if(sscanf(line,"%*s%d\t%*s%d",&var, &off) > 0){
          //fprintf(stderr, "%d - %d\n", var, off);
          fseek(fptr, pos - strlen(line), SEEK_SET);
          fprintf(fptr, "machine_id: %d       \t last_file: %d", var, abs(offset));
          fclose(fptr);
          return 0;
        }
      }
      lin++;
    }
    fclose(fptr);
    return 0;
  }
  else if(ops == 2){
  if(fgets(line, sizeof line, fptr) != NULL) {
      if(sscanf(line,"first_file:%d\tlast_file:%d",&var, &offset) > 0){
        fprintf(fptr, "first_file:%d       \t last_file:%d",mac_id+1, last_file);
      }
      else{
          fprintf(fptr,"%s", line);
      }
  }
  fclose(fptr);
  return 0;
  }
}


int update_broker_offsets_for_topic(char* topic, struct requestmsg *rqst, int isAll){
  char filename[20], line[100];
  sprintf(filename, "%ld/%s/%s.log", myId, topic, topic);
  FILE *fptr = fopen(filename, "r+");
  if(fptr == NULL){
    for(int i = 1; i < 100; i++)
      rqst->mc_offset[i] = 1*isAll;
   // fprintf(stderr, "%d inside %d\n", isAll, rqst->mc_offset[4]);  
    return -1;      
  }
  int lin = 1, var, offset, pos = 0;
  while(fgets(line, sizeof line, fptr) != NULL) {
    pos = ftell(fptr);
    //fprintf(stderr, "%s", line);
    if(sscanf(line,"%*s%d\t%*s%d",&var, &offset) > 0){
      //fprintf(stderr, "%d - %d\n", var, offset);
      rqst->mc_offset[var] = isAll*offset;  
      //fseek(fptr, pos - strlen(line), SEEK_SET);
      //fprintf(fptr, "machine_id: %d       \t last_file: %d", var*100, offset+1000);   
    }
  }
  fclose(fptr);
  return 0;
}



void trim(char * str){
  int index, i;
  index = 0;
  while(str[index] == ' ' || str[index] == '\t' || str[index] == '\n'){
      index++;
  }
  i = 0;
  while(str[i + index] != '\0'){
      str[i] = str[i + index];
      i++;
  }
  str[i] = '\0';
  i = 0;
  index = -1;
  while(str[i] != '\0'){
    if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n'){
      index = i;
    }
    i++;
  }
  str[index + 1] = '\0';
}  