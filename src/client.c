#include "client.h"

int main(int argc, char **argv) {
	char msg_rcvd[BYTES];
	char c, cmd[100], *url, *uri, req[BYTES], *eor, temp[100];
	char *filename, def_file[10];
	FILE *fp;

	int n = 0, cnt, rcvd, rflag, sflag;
	
	struct tms mytms;
	clock_t time_s, time_f;
	double tick = sysconf(_SC_CLK_TCK);

	//struct sockconn serv_info[MAX_CONN];
	if(sock_init() != 0) {
		perror("SOCK_INIT ERROR");
	}

	strcpy(def_port, "8000");
	strcpy(def_file, "temp");
	
	signal(SIGINT, sig_handler);
	

	while(1) {
		// initializing on each line
		strcpy(cmd, "");
		cnt = 0;
		sflag = 0;
		printf(">> ");

		while((c = getchar()) != '\n') {
			cmd[cnt++] = c;
		}
		cmd[cnt] = '\0';
		
		if(strcmp(cmd, "-") == 0) {
			printf("Wrong Command\n");
			continue;
		}
		
		strtok(cmd, " "); // -* option removed
		
		// -p option (default port change)
		if(cmd[1] == 'p') {
			uri = strtok(NULL, " ");
			strcpy(def_port, uri);
			printf("Default Port Changed to No.%s\n", def_port);
			continue;
		}
		else if(cmd[1] == 's') {
			//url = strtok(NULL, " ");
			strcpy(req, "GET /");
			sflag = 1;
		}		
		// -g option
		else if(cmd[1] == 'g') {
			strcpy(req, "GET /");
		}
		// -h option
		else if(cmd[1] == 'h') {
			strcpy(req, "HEAD /");
		}
		else {
			printf("Wrong Command\n");
			continue;
		}

		// request formatting
		url = strtok(NULL, " ");
		if(sflag == 1) {
			filename = strtok(NULL, " ");
			if(!filename) {
				filename = def_file;
			}
		}
		strtok(url, "/");
		uri = strtok(NULL, "/");
		
		strcat(req, uri);
		strcat(req, " HTTP/1.1\r\n");
		
		sprintf(temp, "HOST : %s\r\nConnection : Keep-alive\r\n", url);
		strcat(req, temp);

		// request format test -> should remove at last
		printf("req : %s\n", req);
		
		// if connection full
		if((n = sock_check(url)) == MAX_CONN)
			continue;
		
		// issue request & recv msg
		write(serv_info[n].sockfd, req, strlen(req));
		
		//printf("filename : %s\n", filename);

		if(sflag == 0)
			printf("received msg :\n");
		else if((fp = fopen(filename, "w")) == 0) {
				perror("fopen");
				break;
		}
		
		rflag = 0;
		while((rcvd = read(serv_info[n].sockfd, msg_rcvd, 1024)) > 0) {
			if(eor = strstr(msg_rcvd, "<@EOM@>")) {
				strcpy(eor, "\0");
				rflag = 1;
			}

			if(sflag == 0)
				printf("%s", msg_rcvd);
			if(sflag == 1) {
				if((fwrite(msg_rcvd, 1, rcvd, fp) != rcvd) != (size_t) rcvd) {
					perror("fwrite");
					break;
				}
				/*
				while(strchr(msg_rcvd, '^M')) {
					fprintf(fp, "%s", msg_rcvd);
				}*/
			}
			
			if(rflag == 1)
				break;
		}
		if(sflag == 1) {
			fclose(fp);
			sprintf(cmd, "dos2unix %s", filename);
			system(cmd);
		}

		printf("\n");
		serv_info[n].time_c = times(&mytms);
	}
}

// set all serv_info[] zero
int sock_init() {
	int i;
	for(i = 0; i < MAX_CONN; i++) {
		bzero(&serv_info[i], sizeof(struct sockconn));
	}
	return 0;
}

int sock_check(char *url) {
	int i, cnt = 0;
	char *ip, *port;
	
	ip = strtok(url, ":");
	port = strtok(NULL, ":");
	
	// port check - if no port or wrong port given, take default port
	if(!port)
		port = def_port;
	else if(atoi(port) < 1023 || atoi(port) > 65535) {
		fprintf(stderr, "Wrong Port Number Given!\n");
		port = def_port;
	}

	// first check - if already connected
	for(i = 0; i < MAX_CONN; i++) {
		if(serv_info[i].flag != 0) {
			if(strcmp(serv_info[i].serv_ip, ip) == 0) { // check ip
				if(serv_info[i].serv_addr.sin_port == htons(atoi(port))) // check port
					return i;
				else { // if diff port for existing ip connection, close it
					write(serv_info[i].sockfd, "Close Connection", 16);
					close(serv_info[i].sockfd);
					serv_info[i].flag = 0;
					cnt++;
					printf("reconnect with new port no.\n");
					break;
				}
			}
		}
		else
			cnt++;
	}
	
	// second check - if slot left, unconnected
	if(cnt == 0) {
		printf("Connection is full currently... Checking...\n");
		if(sock_close(0) == -1) { // no connection closed
			printf("Connection not available Now.\n");
			return i; // return MAX_CONN
		}
		else
			printf("Connection available Now.\n");
	}
	
	// make new connection
	for(i = 0; i < MAX_CONN; i++) {
		if(serv_info[i].flag == 0) {
			bzero(&serv_info[i], sizeof(struct sockconn));
			serv_info[i].flag = 1;
			
			// sockaddr setting
			serv_info[i].sockfd = socket(AF_INET, SOCK_STREAM, 0);
			serv_info[i].serv_addr.sin_family = AF_INET;
			serv_info[i].serv_addr.sin_port = htons(atoi(port));
			inet_pton(AF_INET, ip, &(serv_info[i].serv_addr.sin_addr));
			strcpy(serv_info[i].serv_ip, ip);
			
			if(connect(serv_info[i].sockfd, (struct sockaddr *)&(serv_info[i].serv_addr), sizeof(serv_info[i].serv_addr)) == -1) {
				fprintf(stderr, "connection failed\n");
				return MAX_CONN;
			}
			return i;
		}
	}
}

int sock_close(int sig) {
	int i, cnt = -1;
	struct tms mytms;
	clock_t ct = times(&mytms);
	double tick = sysconf(_SC_CLK_TCK);
	
	if(sig == 0) {
		for(i = 0; i < MAX_CONN; i++) {
			if(serv_info[i].flag != 0) {
				if((ct - serv_info[i].time_c) / tick > 120) {
					write(serv_info[i].sockfd, "Close Connection", 16);
					close(serv_info[i].sockfd);
					serv_info[i].flag = 0;
					if(cnt == -1)
						cnt = i;
				}
			}
		}
		return cnt;
	}
	else {
		for(i = 0; i < MAX_CONN; i++) {
			if(serv_info[i].flag != 0) {
				write(serv_info[i].sockfd, "Close Connection", 16);
				close(serv_info[i].sockfd);
			}
		}
		return 0;
	}	
}

void sig_handler() {
	printf("\n\nClosing all connections...\n");
	sock_close(1);
	printf("Client Shutdown\n");
	exit(0);
}
























