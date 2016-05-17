#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <getopt.h>
//#include <vlc/vlc.h>
#include "http_parser.h"

#define BYTES 1024
#define MAX_PATH 1024
#define MAX_MSG 4096
#define MAX_LINE 8192

void server_init(char *);
void parser(int );
void webroot();
void check_clients();
void php_cgi(int , char *);
void parser2(int );
void sig_handler();

typedef struct {
        int maxfd;
        int nready;
        fd_set read_set;
        fd_set ready_set;
} pool;

char *path;
int listenfd;
pool p;

static int on_info(http_parser* p) {
  return 0;
}


static int on_data(http_parser* p, const char *at, size_t length) {
  return 0;
}

static http_parser_settings settings = {
  .on_message_begin = on_info,
  .on_headers_complete = on_info,
  .on_message_complete = on_info,
  .on_header_field = on_data,
  .on_header_value = on_data,
  .on_url = on_data,
  .on_status = on_data,
  .on_body = on_data
};
