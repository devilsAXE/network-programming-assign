/********************************************************************
            File: Publisher
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
#define PORT 12000


int send_message(int, struct message);
void show_menu(int);
int check_topic(char*);

struct message msg;
char topics[50][20];
int topic_idx = 0;
int port_to_connect;

int main(int argc, char *argv[]){

  char ip_brokr[20];
  int sock_fd;
  struct sockaddr_in servaddr;
  fprintf(stderr, "%d\n",argc );
  if(argc > 1 ){
    strcpy(ip_brokr, argv[1]);
    port_to_connect = atoi(argv[2]);
  }
  else{
    fprintf(stderr, "Usage <ip> <port>\n");
    exit(1);
  }
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd < 0) err_sys("socket");

  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port_to_connect);
  servaddr.sin_addr.s_addr = inet_addr(ip_brokr);

  if(connect(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  err_sys("connect");

  //---------- Menu ---------------//
  show_menu(sock_fd);
  return 0;
}

void err_sys(char* x){
    perror(x);
    exit(1);
}

void show_menu(int sock_fd){
  char inbuf [512];
  while(1){
    //sleep(1);
    //clrscr();
    printf("Choose one of the options:\n");
    printf("1. Send [Message] tagged with a [Topic]\n");
    printf("2. Send a [File] tagged under a [Topic] \n");
    int options;
    int len;
    char topic_name[20];
    if (fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
        perror ("fgets");
    sscanf (inbuf, "%d", &options);
    switch (options) {
      case 1: clrscr();
              char topic_name[20];
              char mesg[MAX_MSG_DATA];
              printf("Enter topic: ");
              if(fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
                perror ("fgets");
              len = strlen(inbuf);
              if(inbuf [len - 1] == '\n')
                inbuf [len - 1] = '\0';
              strcpy(msg.topic, inbuf);
              //scanf("%s\n", &(msg)->topic);
              printf("Enter Message: ");
              if(fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
                perror ("fgets");
              len = strlen(inbuf);
              if(inbuf [len - 1] == '\n')
                inbuf [len - 1] = '\0';
              strcpy(msg.content, inbuf);
              msg.machine_id = -getpid();
              msg.offset = 0;
              //scanf("%s\n", &(msg)->content);
              send_message(sock_fd, msg);
              printf("Message Sent\n");
              break;

      case 2: clrscr();
              struct message nrml_msg;
              char filename[20];
              printf("Enter Topic: \n");
              if(fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
                perror ("fgets");
              len = strlen(inbuf);
              if(inbuf [len - 1] == '\n')
                inbuf [len - 1] = '\0';
              strcpy(nrml_msg.topic, inbuf);
              printf("Enter file Path: \n");
              if(fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
                perror ("fgets");
              len = strlen(inbuf);
              if(inbuf [len - 1] == '\n')
                inbuf [len - 1] = '\0';
              strcpy(filename, inbuf);
              FILE *file = NULL;
              unsigned char buffer[MAX_MSG_DATA];  // array of bytes, not pointers-to-bytes
              size_t bytesRead = 0;
              
              int myid = getpid();
              file = fopen(filename, "rb");
              int idx = 0;
              if (file != NULL){
                //fprintf(stderr, "At least Im' in\n");
                while((bytesRead = fread(buffer, 1, sizeof(buffer)-1, file)) > 0){
                  //buffer[strlen(buffer)-1] = '\n';
                  fprintf(stderr,"[STATUS]: block =%d, total_bytes_read = %ld\n", idx, bytesRead);
                  nrml_msg.machine_id = myid;
                  strcpy(nrml_msg.content, buffer);
                  nrml_msg.offset = 1;
                  send_message(sock_fd, nrml_msg);
                  idx++;
                }
                send_message(sock_fd, nrml_msg);
              }
             
              break;

      default: clrscr();
               printf("[STATUS]:Illegal option, try again\n");
               break;
    }
  }
}

int send_message(int sock_fd, struct message msg){
  size_t msg_len = sizeof (struct message);
  if (send (sock_fd, &msg, msg_len, 0) == -1)
    perror ("send");
  return 0;
}

int check_topic(char topic_name[]){
  return 0;
}
