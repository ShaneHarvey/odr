#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "debug.h"


#define SERVER_PATH  "/tmp/server_cse533-14.tmp"
#define ODR_PATH     "/tmp/ODR_cse533-14.tmp"
#define SERVER_PORT 8080

#define MAX_MSGLEN 1024
#define MAX_IPLEN 32

/* Message that the API writes to ODR's UNIX Socket */
/* Message that the ODR writes to the API's UNIX socket (client/server) */
struct api_msg {
    struct in_addr ip;
    int port;
    int flag;
    char msg[MAX_MSGLEN];
};
/* The minimum message is an api_msg with 0 bytes of data */
#define MIN_API_MSG (sizeof(struct api_msg) - MAX_MSGLEN)

int copyhostbyaddr(struct in_addr *ip, char *host, size_t hostlen);
int gethostbystr(char *canonicalIP, char *host, size_t hostlen);

#endif
