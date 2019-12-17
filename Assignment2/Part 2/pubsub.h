#ifndef __PUBSUB_H_
#define __PUBSUB_H_


#define _GNU_SOURCE

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>


#define MAX(a, b) (((a)>(b))?(a):(b))
#define MAX_DATA 1256
#define MAX_MSG_DATA 512
#define MESSAGE_TIME_LIMIT 120
#define NOMSGFOUND -404
#define BULK_LIMIT 10
#define CREAT 9
#define EXPRD 10
#define INFO 11
#define PUB 0  //msg type as well
#define SUB 1
#define BKR 2

struct message{
	int machine_id;
	long offset;
	char topic[20];
	char content[MAX_MSG_DATA];
};

struct requestmsg{
	//int msg_type;		
	char sub_ip[20]; //Contains IP of requesting subscriber
	long sub_port;	// Brkr sets up a connection on this port with 
	int no_of_msg;	// subscriber to send no. of msgs requested
	int brkr_num;
	/************/
	char topic[20];	// Brkr checks the topic 
	int mc_offset[100]; // checks the corresponding offset
	//which is sent by subscriber itself and starts sending message
};
struct querymsg{
	int in_cnt;
	long m_id;
	long offset;
	char topic[20];
};

void err_sys(char*);
static void trim(char *str);
int handle_requests(int, int); // handle request from pub, sub, brkr
static void zombie_cleaner(int);
int push_messages(int, struct message);
int log_operations(char*, int, long*, long*); // f(file, ops, val1, val2)

#endif
