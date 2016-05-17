#include "server.h"

//static http_parser hparser;

/*
http_parser parser;
http_parser_init(parser, HTTP_REQUEST);
parser->data = my_socket;

struct http_parser hparser;
http_parser_init(&hparser, HTTP_REQUEST);
*/

int main(int argc, char* argv[]) {
	struct sockaddr_in clientaddr;
    	socklen_t addrlen;
	char c, PORT[6] = "8000";
	int i, connfd;

	// getopt arguments
	static struct option long_options[] = {
		{"root", 1, NULL, 'r'},
		{"port", 1, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};
	int option_index = 0;
	struct stat s;

	signal(SIGINT, sig_handler);

	// Default Port = 8000 / path = cwd
	path = (char*)malloc(sizeof(char) * MAX_PATH);
	webroot();

	// HTTP Parser initialization
	/*
	http_parser_settings settings;
	http_parser_settings_init(settings);
	http_parser *parser = (http_parser*)malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	*/
    	//Parsing the command line arguments
    	while((c = getopt_long_only(argc, argv, "p:r:", long_options, &option_index)) != -1) {
		switch(c) {
	    		case 'r':
				if(stat(optarg, &s) == -1) {
					if(errno == ENOENT) {
						fprintf(stderr, "Wrong Path Given!\n");
					}
					else {
						perror("stat");
					}
					break;
				}
				else if(S_ISDIR(s.st_mode)) {
					strcpy(path,optarg);
				}
				break;
	    		case 'p':
				if(atoi(optarg) < 1023 || atoi(optarg) > 65535) {
					fprintf(stderr, "Wrong Port Number Given!\n");
				}
				else {
					strcpy(PORT,optarg);
				}
				break;
	    		case '?':
				fprintf(stderr,"Wrong arguments given!!!\n");
				exit(1);
	    		default:
				exit(1);
		}
	}

	printf("HTTP Server Start\nPort No. - %s%s%s\nRoot directory - %s%s%s\n",
	    		"\033[92m",
    			PORT,
    			"\033[0m",
    			"\033[92m",
    			path,
    			"\033[0m"
	      );

	server_init(PORT);

    	// ACCEPT connections
    	while(1) {
		p.ready_set = p.read_set;
		p.nready = select(p.maxfd + 1, &p.ready_set, NULL, NULL, NULL);

		addrlen = sizeof(clientaddr);		
    		
		if(FD_ISSET(listenfd, &p.ready_set)) {
			connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &addrlen);
    			if(connfd < 0)
				perror("ACCEPT ERROR");
			
			FD_SET(connfd, &p.read_set);
			if(connfd > p.maxfd)
				p.maxfd = connfd;
			p.nready--;
			printf("connection established - socket %d\n", connfd);
		}
		
		check_clients();
	}
    	return 0;
}

//start server
void server_init(char *port) {
    	struct addrinfo hints, *res, *paddr;
	
    	// getaddrinfo for host
	bzero(&hints, sizeof(hints));
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_PASSIVE;
    	if(getaddrinfo(NULL, port, &hints, &res) != 0) {
		perror("GETADDRINFO ERROR");
		exit(1);
    	}

    	// socket and bind
    	for(paddr = res; paddr != NULL; paddr = paddr->ai_next) {
		listenfd = socket(paddr->ai_family, paddr->ai_socktype, 0);
		if(listenfd == -1)
			continue;
		if(bind(listenfd, paddr->ai_addr, paddr->ai_addrlen) == 0)
			break;
    	}
    	if(paddr == NULL) { // socket / bind error control
		perror("SOCKET / BIND ERROR");
		exit(1);
    	}
	
    	freeaddrinfo(res);
	
    	// listen for incoming connections
    	if(listen(listenfd, 1000) != 0) {
		perror("LISTEN ERROR");
		exit(1);
    	}

	// initialize pool
	p.maxfd = listenfd;
	FD_ZERO(&p.read_set);
	FD_SET(listenfd, &p.read_set);
}
/*
void parser2(int cfd) {
	char buf[MAX_MSG];
	int rcvd, nparsed, fd, bytes_read;
	
	hparser->data = cfd;
	rcvd = recv(cfd, buf, MAX_MSG, 0);
	
	if(rcvd < 0)
		fprintf(stderr, "RECEIVE ERROR\n");
	nparsed = http_parser_execute(hparser, &settings, buf, rcvd);
	
	if(nparsed != rcvd)
		fprintf(stderr, "PARSER ERROR\n");
}
*/


//client connection
void parser(int cfd) {
    	char mesg[MAX_MSG], *reqline[3], data_to_send[BYTES], *s;
    	int rcvd, fd, bytes_read;
	int hflag = 1; // default : HTTP/1.1	

    	memset((void*)mesg, (int)'\0', MAX_MSG);
	
    	rcvd = read(cfd, mesg, MAX_MSG);
	
    	if(rcvd < 0)
		fprintf(stderr, "RECEIVE ERROR\n");
    	else if(rcvd == 0) {
		fprintf(stderr, "Unexpected Disconnection.\n");
	}
	else if(strcmp(mesg, "Close Connection") == 0) {
		printf("Connection Closed - Socket %d\n", cfd);
		shutdown(cfd, SHUT_RDWR);
		close(cfd);
		FD_CLR(cfd, &p.read_set);
		if(cfd == p.maxfd) {
			p.maxfd--;
			while(!FD_ISSET(p.maxfd, &p.read_set))
				p.maxfd--;
		}
	}	
    	else {   // message received
    		printf("%s", mesg);
		reqline[0] = strtok(mesg, " \t\n");
		reqline[1] = strtok(NULL, " \t");
		reqline[2] = strtok(NULL, " \t\n");
		if(strncmp(reqline[2], "HTTP/1.0", 8) == 0)
			hflag = 0;
		else if(strncmp(reqline[2], "HTTP/1.1", 8) == 0)
			hflag = 1;
		if (strncmp(reqline[1], "/\0", 2) == 0)
			reqline[1] = "/index.html";
		// index.html will be opened by default

		webroot();
		strcpy(&path[strlen(path)], reqline[1]);
		printf("file: %s\n\n", path);
		
		if((fd = open(path, O_RDONLY)) != -1) {   //FILE FOUND
			if(hflag == 0)
				write(cfd, "HTTP/1.0 200 OK\n", 16);
			else if(hflag == 1)
				write(cfd, "HTTP/1.1 200 OK\n", 16);
		}
		else {  // FILE NOT FOUND
			if(hflag == 0)
				write(cfd, "HTTP/1.0 404 Not Found\n", 23);
			else if(hflag == 1)
				write(cfd, "HTTP/1.1 404 Not Found\n", 23);
		}


		if(strncmp(reqline[0], "GET\0", 4) == 0) {
			/* php handler
			s = strchr(path, '.');
			if(strcmp(s + 1, "php") == 0) {
				php_cgi(cfd, reqline[1]);
			}
			*/
			while((bytes_read = read(fd, data_to_send, BYTES)) > 0)
				write(cfd, data_to_send, bytes_read);
		}
		else if(strncmp(reqline[0], "HEAD\0", 5) == 0) {
			write(cfd, "HEAD REQUEST ACCEPTED\n", 22); 
		}
		write(cfd, "<@EOM@>", 7);
    	}
	
	// Somewhere makes /Sample.html duplicate -> need to fix
	// Guess reqline & path of global var setting problem
    	// SOCKET Close back in check_clients function
}

void webroot() {
	FILE *fp;
	
	if(fp = fopen("conf", "rt")) {
		fgets(path, MAX_PATH, fp);
		fclose(fp);
	}
	else
		strcpy(path, getenv("PWD"));
}

void check_clients() {
	int s;
	
	for(s = 0; (s < p.maxfd + 1) && (p.nready > 0); s++) {
		if(s == listenfd)
			continue;
		
		if(FD_ISSET(s, &p.read_set) && FD_ISSET(s, &p.ready_set)) {
			p.nready--;
			parser(s);
			
		  	//Further sent and received operations are DISABLED
			//shutdown(s, SHUT_RDWR);
			//close(s);
			FD_CLR(s, &p.ready_set);
			//if(s == p.maxfd) {
			//	p.maxfd--;
			//	while(!FD_ISSET(p.maxfd, &p.read_set))
			//		p.maxfd--;
			//}
		}
	}
}

void php_cgi(int fd, char *s) {
        dup2(fd, STDOUT_FILENO);
        char script[500];
	strcpy(script, "SCRIPT_FILENAME=");
	strcat(script, path);
	putenv(script);
	//strcpy(script, "SCRIPT_NAME=");
	//strcat(script, s);
	//putenv(script);
	putenv("GATEWAY_INTERFACE=CGI/1.1");
	//putenv("QUERY_STRING=");
	//putenv("PATH_INFO=/");
	putenv("REQUEST_METHOD=GET");
	putenv("REDIRECT_STATUS=true");
	putenv("SERVER_PROTOCOL=HTTP/1.1");
	//strcpy(script, "REQUEST_URL=");
	//strcat(script, s);
	//putenv(script);
	//putenv("HTTP_HOST=localhost");
	//putenv("REMOTE_HOST=127.0.0.1");
	execl("/usr/bin/php-cgi", "php-cgi", NULL);
}

void sig_handler() {
	int i;
	printf("\n\nClosing all connections...\n");
	for(i = 0; (i < p.maxfd + 1) && (p.nready > 0); i++) {
		if(FD_ISSET(i, &p.read_set)) {
			p.nready--;
			shutdown(i, SHUT_RDWR);
			close(i);
			FD_CLR(i, &p.read_set);
		}
	}
	printf("Server Shutdown\n");
	exit(0);
}
