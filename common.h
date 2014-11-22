#ifndef COMMON_H
#define COMMON_H
/* libc headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
/* System headers */
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
/* Program headers */
#include "debug.h"

#define SERVER_PATH  "/tmp/server_cse533-14.tmp"
#define ODR_PATH     "/tmp/ODR_cse533-14.tmp"
#define SERVER_PORT 8080

#define MAX_MSGLEN 1470
#define MAX_IPLEN 32

/* Message that the API writes to ODR's UNIX Socket */
/* Message that the ODR writes to the API's UNIX socket (client/server) */
struct api_msg {
    struct in_addr ip;      /* Source/Destination IP */
    int port;               /* Source/Destination Port */
    int flag;               /* Force RREQ flag for sending to ODR */
    char msg[MAX_MSGLEN];   /* Application message */
};
/* The minimum message is an api_msg with 0 bytes of data */
#define MIN_API_MSG (sizeof(struct api_msg) - MAX_MSGLEN)

int copyhostbyaddr(struct in_addr *ip, char *host, size_t hostlen);
int gethostbystr(char *canonicalIP, char *host, size_t hostlen);
int getipbyhost(char *hostname, struct in_addr *hostip);

#endif
