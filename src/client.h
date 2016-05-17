#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <signal.h>

#define MAX_CONN 10
#define BYTES 1024

char def_port[6];

struct sockconn {
	int sockfd;
	struct sockaddr_in serv_addr;
	char serv_ip[15];
	clock_t time_c;
	int flag;
};

struct sockconn serv_info[MAX_CONN];

int sock_init();
int sock_check(char *url);
int sock_close(int sig);
void sig_handler();
