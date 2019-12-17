/********************************************************************
            	File: -  BROKERv1.0
            Features: 1. Serve message/relay requests
                      2. Relay message requests
                      3. Delete messages periodically

********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <time.h>
#include <pthread.h> 
#include "pubsub.h"

#define PUBPORT 12000
#define SUBPORT 12001
#define BRKRPORT 12003
#define BACKLOG 10
#define BID 1
#define CHECK_INTERVAL 5


int connectToAnotherBroker(char*, int);
int service_type(int, int*);
void show_menu();
int send_req_neighbour(struct requestmsg);

static int broker_available = 1;
int brkr_fd;
int user_port = 0;
int neighbour_port = 0;
const char* Service[] = {"Publisher", "Subscriber", "Broker"};
int brkr_num;
const char *timefile = "info.time";
struct timeinfo{
	time_t seconds;
	char msgfile[20];
	char logfile[20];
};

void *garbage_cleaner(void *vargp){
	FILE* timeptr = NULL;					
	timeptr = fopen(timefile, "wb+");
	if(timeptr == NULL){
		perror("fwrite");
	}
	fclose(timeptr);
	FILE* time_ptr = NULL;
	//fprintf(stderr,"Status: Garbage Cleaning Service Active\n");
	struct timeinfo t_info;
	int first_file, last_file;
	char line[100];
	int deleted = 0, delete_dir = 0;
	while(1){
		time_t curr = time(NULL);
		struct timeinfo deleted_info[100];
		fprintf(stderr, "Time Now: %ld\n", curr);
		time_ptr = fopen(timefile, "rb");
		if(time_ptr == NULL){
			fprintf(stderr,"Status: Garbage Cleaning Service Unavailable\n");
			exit(EXIT_FAILURE);
		}

		fread(deleted_info, sizeof(struct timeinfo), deleted, time_ptr); //Hack to set pointer to undeleted file's info.time log 
		while(fread(&t_info, sizeof(struct timeinfo), 1, time_ptr)){
			delete_dir = 0;
			//fprintf(stderr,"-----------------------------> File Saved @ %ld\n", t_info.seconds);
			if(curr - (t_info.seconds) >= MESSAGE_TIME_LIMIT){
				FILE *fptr = fopen(t_info.logfile, "r+");
				if(fptr == NULL){
					//perror("Log File");
					break;
				}
				fseek(fptr, 0, SEEK_SET);
				if(fgets(line, sizeof line, fptr) != NULL) {
          			if(sscanf(line,"first_file:%d\tlast_file:%d",&first_file, &last_file) > 0){
            			fseek(fptr, 0, SEEK_SET);
            			fprintf(fptr, "first_file:%d\t last_file:%d\n",first_file+1, last_file);
            			if(first_file+1 == last_file) delete_dir = 1;
          			}
        		}
     			fclose(fptr);
				if(remove(t_info.msgfile) < 0){
					perror("remove");
					break;
				}
				else{
					deleted++;
					fprintf(stderr,"[STATUS]: %s deleted and unlinked\n", t_info.msgfile);
				} 
			
			}
			
		}
		fclose(time_ptr);
		sleep(CHECK_INTERVAL);
	}
	
}



	

int main(int argc , char *argv[]){
	
	long service_ports[3] = {PUBPORT, SUBPORT, BRKRPORT};
	if (argc > 1){
		user_port = atol(argv[1]);
		neighbour_port = atol(argv[2]);
	}
	if (user_port){
		service_ports[0] = user_port;
		service_ports[1] = user_port + 1;
		service_ports[2] = user_port + 2;
	}
	char inbuf [2];
	fprintf(stderr,"Enter Broker No(1-100):>");
	if (fgets (inbuf, sizeof (inbuf),  stdin) == NULL)
        perror ("fgets");
    sscanf (inbuf, "%d", &brkr_num);
	fprintf(stderr,"Broker No(1-100):> %d\n", brkr_num);

	broker_available = 0;
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = zombie_cleaner;

	if(sigaction(SIGCHLD, &sa, NULL) == -1){
	    fprintf(stderr, "ERROR from sigaction %s\n", strerror(errno));
	}

	int listenfd[3], connfd[3], maxfd[3] = {-1, -1, -1};
	struct sockaddr_in servaddr[3], clientaddr;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int optval = 1;
	fd_set master_rfds, master_wfds, fds;

	/*Setting up the listening ports*/
	//printf("Setting up PORTS\n");
	pid_t pids[3];

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	for (int chld = 0 ; chld < 3; chld++){
		pids[chld] = fork(); 
		if(pids[chld] == 0){
			broker_available = 1;
			if((listenfd[chld] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				err_sys("Socket");
			setsockopt(listenfd[chld], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
			memset(&servaddr[chld], 0, sizeof(servaddr[chld]));
			servaddr[chld].sin_port = htons(service_ports[chld]);
			servaddr[chld].sin_family = AF_INET;
			servaddr[chld].sin_addr.s_addr = htonl(INADDR_ANY);

			if((bind(listenfd[chld], (struct sockaddr*)&servaddr[chld], sizeof(servaddr[chld]))) < 0)
				err_sys("Bind");

			if(listen(listenfd[chld], BACKLOG) < 0)
				err_sys("Listen");
			fprintf(stderr, "Successfully listening at: [%ld] by Process: [%d]\n", service_ports[chld], getpid());

			maxfd[chld] = MAX(maxfd[chld], listenfd[chld]);
			FD_ZERO(&master_rfds); FD_ZERO(&master_wfds);
			int fd_new, nready;
			for(;;){
				master_rfds = fds;
				FD_SET(listenfd[chld], &master_rfds); FD_SET(listenfd[chld], &master_wfds);
				if(select(maxfd[chld] + 1, &master_rfds, &master_wfds, NULL, &tv) < 0)
					err_sys("select");
				for(int fd = 0; fd <= maxfd[chld]; fd++){
					if(FD_ISSET(fd, &master_rfds)){
						//printf("FD SET: %d\n", fd);
						if(fd == listenfd[chld]){
							addrlen = sizeof clientaddr;
							fd_new = accept(listenfd[chld], (struct sockaddr*)&clientaddr,
								&addrlen);
							if(fd_new < 0) err_sys("accept");
							FD_SET(fd_new, &fds);
							maxfd[chld] = MAX(maxfd[chld], fd_new);
							fprintf(stderr, "[STATUS]: %s [%s] connected at port = %ld\n", Service[chld],
								inet_ntoa(clientaddr.sin_addr), service_ports[chld]);
						
						}
						else{
							if(handle_requests(fd, chld) < 0){
								FD_CLR(fd, &fds);
								close(fd);
							}
						}
					}
				}
			}
		close(listenfd[chld]);
		exit(EXIT_SUCCESS);
		}
		
	}

	if(pids[0] > 0 && pids[1] && pids[2] > 0){
		pthread_t gc_id;
		pthread_create(&gc_id, NULL, garbage_cleaner, NULL);
		pthread_join(gc_id, NULL);
	}
	
	for(int i = 0 ; i < 3; i++){
		int status;
		wait(&status);
	}
	//munmap(broker_available, sizeof broker_available);
}
	
int handle_requests(int socket_fd, int service){
	
	struct requestmsg qmsg;
	struct message nrml_msg;
	struct timeinfo t_info;
	char logfile[20];
	ssize_t num_read;
	long first_file = 1, last_file = 1, mac_id, offset;
	char *recv_topic, *recv_content;
	memset(&qmsg, '\0', sizeof(struct querymsg));
	memset(&nrml_msg, '\0', sizeof(struct message));
	//printf("I'M HANDLING %d\n", service);
	char rqst_ip[20];
	long rqst_port;
	int rqst_msg_tot, rqst_offset, org_brkr;
	char *topic;
	switch(service){
		case 0: // Recv message and topic to save them in directory
				while(recv(socket_fd, &nrml_msg, sizeof(struct message), 0) > 0 && nrml_msg.offset != -99){
					mac_id = nrml_msg.machine_id;
					offset = nrml_msg.offset;
					recv_topic = (char *)malloc(strlen(nrml_msg.topic) + 1);
					recv_content = (char *)malloc(strlen(nrml_msg.content) + 1);
					strcpy(recv_topic, nrml_msg.topic); trim(recv_topic);
					strcpy(recv_content, nrml_msg.content);trim(recv_content);
					memset(&t_info, '\0', sizeof t_info);

					fprintf(stderr,"\n[STATUS]: Message on topic = '%s' received from = %d of length = %ld\n",  recv_topic, abs(mac_id), strlen(recv_content));
					
					/*create a directory if not exists */
					sprintf(logfile, "%s/%s.log", recv_topic, recv_topic);
					if(mkdir(recv_topic, 0777) == -1){
						perror("Topic Create");	
					}else{ // New Topic Dir Created -> Create the LOG file 
						FILE *fptr = fopen(logfile, "w+");
						fprintf(fptr, "first_file:%d\tlast_file:%d\n", 1, 1);
						fclose(fptr);
					}
					/*--Directory / Log file Created (if reqd)--*/
					FILE* fptr;
					char msgfile[20];
					// This operation gets the offset we need to save the message as
					log_operations(recv_topic, INFO, &first_file, &last_file); 
					sprintf(msgfile, "%s/%ld.dat", recv_topic, last_file);
					if((fptr = fopen(msgfile, "w")) == NULL)
						err_sys("PUB fopen");
					fputs(recv_content, fptr);
					log_operations(recv_topic, CREAT, &first_file, &last_file);
					fclose(fptr);
					fprintf(stderr,"[STATUS]: %s SAVED\n",recv_topic);

					t_info.seconds = time(NULL);
					strcpy(t_info.msgfile, msgfile);
					strcpy(t_info.logfile, logfile);

					FILE* timeptr = NULL;					
					timeptr = fopen(timefile, "ab+");
					if(timeptr == NULL){
						perror("fwrite");
					}
					fwrite(&t_info, sizeof(struct timeinfo),1, timeptr);
					fclose(timeptr);
					free(recv_topic);free(recv_content);
					if(mac_id < 0){
						//printf("Had enough!\n");
						return 0;
					} 
				}
					fprintf(stderr,"[STATUS]: Connection closed by: %d\n", socket_fd);
					return -1;
				/*************************** END OF PUB ***************************/

		case 1: // Recv request from subscriber
				//struct requestmsg qmsg;
				num_read = recv(socket_fd, &qmsg, sizeof(struct requestmsg), 0);
				if(num_read > 0){
					strcpy(rqst_ip, qmsg.sub_ip);
					rqst_port = qmsg.sub_port;
					recv_topic = (char *)malloc(strlen(qmsg.topic) + 1);
					strcpy(recv_topic, qmsg.topic); trim(recv_topic);
					rqst_offset = qmsg.mc_offset[brkr_num];
					qmsg.brkr_num = brkr_num;
					fprintf(stderr,"\n[REQUEST]: from = '%s@%ld', topic = '%s', offset = '%d' \n", rqst_ip, rqst_port, recv_topic, rqst_offset);
					
					/***************Check if needs to download from the whole system***********************/
					if(rqst_offset < 0){
						fprintf(stderr, "[STATUS]: Request for topic='%s' relaying throughout the system... \n", recv_topic);
						qmsg.brkr_num = brkr_num;
						if(send_req_neighbour(qmsg) > 0){
						
						}
						int err = log_operations(recv_topic, INFO, &first_file, &last_file);
						rqst_offset = first_file; //Sending from local disk
						//fprintf(stderr, "Rqst Offset Now:%d & Last File:%ld\n", rqst_offset, last_file);
						while(rqst_offset < last_file && err >= 0){					
							memset(&nrml_msg, '\0', sizeof(struct message));
							char filetosend[50];
							sprintf(filetosend, "%s/%d.dat", recv_topic, rqst_offset);
							FILE *fptr = fopen(filetosend, "r");
							if(fptr == NULL){
								perror("fopen filetosend");
								memset(&nrml_msg, '\0', sizeof(struct message));
								nrml_msg.machine_id = brkr_num;
								nrml_msg.offset = rqst_offset;;
								strcpy(nrml_msg.topic,"NOTFOUND");
								strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
								if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
									perror ("sub send");
								return 0;
							}
							fseek(fptr, 0, SEEK_END);
							long fsize = ftell(fptr);fseek(fptr, 0, SEEK_SET);
							char *content = (char*)malloc(fsize + 1);
							fread(content, 1, fsize, fptr);
							fclose(fptr);content[fsize] = 0; trim(content);
							strncpy(nrml_msg.content, content, fsize+1);free(content);
							nrml_msg.machine_id = brkr_num;
							if(rqst_offset == last_file - 1)
							nrml_msg.machine_id = -brkr_num; // Indicates ending of a message (1 msg)
							strcpy(nrml_msg.topic, recv_topic);
							nrml_msg.offset = rqst_offset;
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("send");
							fprintf(stderr,"[STATUS]: Message Successfully Sent\n");
							rqst_offset++;
						}
						if(err < 0){
							memset(&nrml_msg, '\0', sizeof(struct message));
							nrml_msg.machine_id = brkr_num;
							nrml_msg.offset = -rqst_offset;;
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							return 0;
						}
						return 0;					
					}
					/****************************************************************************************/
					
					/* Check if we have this message */

					int err = log_operations(recv_topic, INFO, &first_file, &last_file); 
					if(err < 0){
						fprintf(stderr, "[STATUS]: topic = '%s' not available, querying neighbour broker.\n", recv_topic); 
						if(send_req_neighbour(qmsg) > 0){
							memset(&nrml_msg, '\0', sizeof(struct message));
							//fprintf(stderr, "QUERYING SERVICE AVAILABLE\n");
							nrml_msg.machine_id = brkr_num; // same m_id means message not there
							nrml_msg.offset = -rqst_offset; //-ve offset indicates to Subscriber
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG QUERIED");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							return 0;
						}
						else{
							memset(&nrml_msg, '\0', sizeof(struct message));
							fprintf(stderr, "[STATUS]: Querying service unavailable\n");
							nrml_msg.machine_id = brkr_num; 
							nrml_msg.offset = -rqst_offset; 
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
      							perror ("sub send");
							return 0;
						}
					}
					else{
						//fprintf(stderr, "Requested Off [%d] -  Available [%ld]", rqst_offset, last_file);
						if(rqst_offset < first_file ) rqst_offset = first_file;
						if(rqst_offset > last_file) rqst_offset = last_file;
					
						memset(&nrml_msg, '\0', sizeof(struct message));
						char filetosend[50];
						sprintf(filetosend, "%s/%d.dat", recv_topic, rqst_offset);
						FILE *fptr = fopen(filetosend, "r");
						if(fptr == NULL){
							perror("fopen filetosend");
							memset(&nrml_msg, '\0', sizeof(struct message));
							//fprintf(stderr, "QUERYING SERVICE UNAVAILABLE\n");
							nrml_msg.machine_id = brkr_num; // same m_id means message not there
							nrml_msg.offset = -rqst_offset;;
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER!");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							return 0;
						}
						fseek(fptr, 0, SEEK_END);
						long fsize = ftell(fptr);fseek(fptr, 0, SEEK_SET);
						char *content = (char*)malloc(fsize + 1);
						fread(content, 1, fsize, fptr);
						fclose(fptr);content[fsize] = 0; trim(content);
						strncpy(nrml_msg.content, content, fsize+1);free(content);

						nrml_msg.machine_id = -brkr_num; // Indicates ending of a message (1 msg)
						strcpy(nrml_msg.topic, recv_topic);
						nrml_msg.offset = rqst_offset;

						if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
							perror ("send");
						fprintf(stderr,"[STATUS]: Message Successfully Sent\n");
						return 0;
					}
				}
				else if(num_read == 0){
					fprintf(stderr,"[STATUS]: Connection Closed by Subscriber\n");
					return -1;
				}
				else{
					return -1;
				}
				break;
				/*************************** END OF SUB ***************************/
		
		case 2:// We handle request from neighbouring Broker, looks much like SUB 
				// Will try to make a common function except the part where we give BULK_LIMIt
				// messages instead of just one 
				//struct requestmsg qmsg;
				num_read = recv(socket_fd, &qmsg, sizeof(struct requestmsg), 0);
				if(num_read > 0){
					if(qmsg.brkr_num == brkr_num){
						fprintf(stderr, "[STATUS]: Stopped Querying Further\n");
						return -1;
					}
					strcpy(rqst_ip, qmsg.sub_ip);
					rqst_port = qmsg.sub_port;
					recv_topic = (char *)malloc(strlen(qmsg.topic) + 1);
					strcpy(recv_topic, qmsg.topic); trim(recv_topic);
					rqst_offset = qmsg.mc_offset[brkr_num];
					fprintf(stderr,"\n[REQUEST]: from = '%s@%ld', topic = '%s', offset = '%d' \n", rqst_ip, rqst_port, recv_topic, rqst_offset);
					close(socket_fd);
					socket_fd = connectToAnotherBroker(rqst_ip, rqst_port);
					if(socket_fd < 0){
						fprintf(stderr, "[STATUS]: Couldn't Connect to Subscriber. Closing...\n");
						close(socket_fd);
						return -1;
					}
					/***************Check if needs to download from the whole system***********************/
					if(rqst_offset < 0){
						fprintf(stderr, "[STATUS]: Request for topic='%s' relaying throughout the system... \n", recv_topic);
						if( send_req_neighbour(qmsg) > 0){ // Request sent 
					
						}
						
						int err = log_operations(recv_topic, INFO, &first_file, &last_file);
						rqst_offset = first_file; //Sending from local disk
						rqst_offset = abs(rqst_offset); //Sending from local disk
						int msg_limit = 0;
						while(rqst_offset < last_file && err >= 0 && msg_limit <= BULK_LIMIT){

							memset(&nrml_msg, '\0', sizeof(struct message));
							char filetosend[50];
							sprintf(filetosend, "%s/%d.dat", recv_topic, rqst_offset);
							FILE *fptr = fopen(filetosend, "r");
							if(fptr == NULL){
								perror("fopen filetosend");
								memset(&nrml_msg, '\0', sizeof(struct message));
								nrml_msg.machine_id = brkr_num;
								nrml_msg.offset = rqst_offset;;
								strcpy(nrml_msg.topic,"NOTFOUND");
								strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
								if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
									perror ("sub send");
								return -1;
							}
							fseek(fptr, 0, SEEK_END);
							long fsize = ftell(fptr);fseek(fptr, 0, SEEK_SET);
							char *content = (char*)malloc(fsize + 1);
							fread(content, 1, fsize, fptr);
							fclose(fptr);content[fsize] = 0; trim(content);
							strncpy(nrml_msg.content, content, fsize+1);free(content);
							nrml_msg.machine_id = brkr_num;
							// Indicates ending of a message (1 msg)
							strcpy(nrml_msg.topic, recv_topic);
							nrml_msg.offset = rqst_offset;
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("send");
							fprintf(stderr,"[STATUS]: Message Successfully Sent\n");
							rqst_offset++;
							msg_limit++;
						}
						if(err < 0){
							memset(&nrml_msg, '\0', sizeof(struct message));
							nrml_msg.machine_id = brkr_num;
							nrml_msg.offset = rqst_offset;;
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							close(socket_fd);	
							return -1;
						}
						close(socket_fd);
						return -1;					
					}
					/****************************************************************************************/
					
					/* Check if we have this message */

					 int err = log_operations(recv_topic, INFO, &first_file, &last_file); 
					if(err < 0){
						fprintf(stderr, "[%s] TOPIC NOT AVAILABLE! QUERYING NEIGHBOURING BROKER\n", recv_topic); 
						if(send_req_neighbour(qmsg) > 0){
							memset(&nrml_msg, '\0', sizeof(struct message));
							//fprintf(stderr, "QUERYING SERVICE AVAILABLE\n");
							nrml_msg.machine_id = brkr_num; // same m_id means message not there
							nrml_msg.offset = -rqst_offset; //-ve offset indicates to Subscriber
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG QUERIED");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							return -1;
						}
						else{
							memset(&nrml_msg, '\0', sizeof(struct message));
							fprintf(stderr, "QUERYING SERVICE UNAVAILABLE\n");
							nrml_msg.machine_id = brkr_num; 
							nrml_msg.offset = -rqst_offset; 
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER! QUERYING UNAVAILABLE");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
      							perror ("sub send");
							return -1;
						}
					}
					
					else{
						
						if(rqst_offset < first_file ) rqst_offset = first_file;
						if(rqst_offset > last_file) rqst_offset = last_file;
						memset(&nrml_msg, '\0', sizeof(struct message));
						char filetosend[50];
						sprintf(filetosend, "%s/%d.dat", recv_topic, rqst_offset);
						FILE *fptr = fopen(filetosend, "r");
						if(fptr == NULL){
							perror("fopen filetosend");
							memset(&nrml_msg, '\0', sizeof(struct message));
							//fprintf(stderr, "QUERYING SERVICE UNAVAILABLE\n");
							nrml_msg.machine_id = brkr_num; // same m_id means message not there
							nrml_msg.offset = -rqst_offset;;
							strcpy(nrml_msg.topic,"NOTFOUND");
							strcpy(nrml_msg.content, "MSG NOT ON THIS BROKER!");
							if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
								perror ("sub send");
							return 0;
						}
						fseek(fptr, 0, SEEK_END);
						long fsize = ftell(fptr);fseek(fptr, 0, SEEK_SET);
						char *content = (char*)malloc(fsize + 1);
						fread(content, 1, fsize, fptr);
						fclose(fptr);content[fsize] = 0; trim(content);
						strncpy(nrml_msg.content, content, fsize+1);free(content);

						nrml_msg.machine_id = -brkr_num; // Indicates ending of a message (1 msg)
						strcpy(nrml_msg.topic, recv_topic);
						nrml_msg.offset = rqst_offset;

						if (send (socket_fd, &nrml_msg, sizeof(struct message), 0) == -1)
							perror ("send");
						fprintf(stderr,"[STATUS]: Message Successfully Sent\n");
						return 0;
					}
				}
				else if(num_read == 0){
					fprintf(stderr,"[STATUS]: Connection Closed by Subscriber\n");
					return -1;
				}
				else{
					return -1;
				}
				break;
	}
	
}

int connectToAnotherBroker(char *ip_brokr, int n_port){
    struct sockaddr_in servaddr;
	
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	//fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(n_port);
    servaddr.sin_addr.s_addr = inet_addr(ip_brokr);
	if (connect(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
      perror("connect brkr");
	  return -1;
    }
	//fprintf(stderr, "Status: Connection to Neighbour Broker Successful @%d \n", sock_fd);
    broker_available = 1;
   	return sock_fd;
 }


 int log_operations(char* dir, int ops, long* first_file, long* last_file){
    char line[MAX_MSG_DATA];
    size_t len = 0;
    ssize_t read;
    char filename[20];
    sprintf(filename, "%s/%s.log", dir, dir);
    FILE *fptr = fopen(filename, "r+");
    if(fptr == NULL){
      perror("log fopen");
      return -1;
    }
    //long first_file, last_file;
    fseek(fptr, 0, SEEK_SET);
    if(ops == INFO){
      if(fgets(line, sizeof line, fptr) != NULL) {
          sscanf(line,"first_file:%ld\tlast_file:%ld",first_file, last_file);
      }
      fclose(fptr);
        return 0;
    }
    else if(ops == CREAT){
      if(fgets(line, sizeof line, fptr) != NULL) {
          if(sscanf(line,"first_file:%ld\tlast_file:%ld",first_file, last_file) > 0){
            fseek(fptr, 0, SEEK_SET);
            fprintf(fptr, "first_file:%ld\t last_file:%ld\n",*first_file, *last_file+1);
          }
        }
      fclose(fptr);
      return 0;
    }
    else if(ops == EXPRD){
      if(fgets(line, sizeof line, fptr) != NULL) {
          if(sscanf(line,"first_file:%ld\tlast_file:%ld",first_file, last_file) > 0){
            fseek(fptr, 0, SEEK_SET);
            fprintf(fptr, "first_file:%ld\t last_file:%ld\n",*first_file+1, *last_file);
          }
        }
      fclose(fptr);
      return 0;
    }

  }


void err_sys(char *err){
  	perror(err);
  	exit(EXIT_FAILURE);
}

static void trim(char * str){
	int index, i;
	index = 0;
	while(str[index] == ' ' || str[index] == '\t' || str[index] == '\n')index++;
	i = 0;
	while(str[i + index] != '\0'){
	    str[i] = str[i + index];
	    i++;
	}
	str[i] = '\0';i = 0;index = -1;
	while(str[i] != '\0'){
	  if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n'){
	    index = i;
	  }i++;
	}str[index + 1] = '\0';

}

static void zombie_cleaner(int sig){
	while(waitpid(-1, NULL, 0) > 0){
	  fprintf(stderr, "[STATUS]:Client Disconnected ----- \n");
	  continue;
	}
  }

int send_req_neighbour(struct requestmsg qmsg){
	int brkr_fd = 0;
	char *ip_brokr = "127.0.0.1";
	brkr_fd = connectToAnotherBroker(ip_brokr, neighbour_port);
	if(brkr_fd < 0){
		fprintf(stderr, "[STATUS]:Querying Service Inactive\n");
		broker_available = 0;
	}
	else{
		broker_available = 1;
	}
	if(broker_available){ // sending a request ahead
		//qmsg.brkr_num = brkr_num;
		fprintf(stderr," [STATUS]: Querying Neighbouring Broker \n");
		if (send (brkr_fd, &qmsg,  sizeof(struct requestmsg), 0) == -1) // Request Neigbour to send N messages to Our Subscriber
			perror ("sub query send");
		close(brkr_fd);
		return 1;
	}
	return -1; //not available broker

}