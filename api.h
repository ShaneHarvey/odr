#ifndef API_H
#define API_H
/* libc headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
/* System headers */
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
/* Program headers */
#include "debug.h"
#include "common.h"

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

ssize_t msg_send(int sd, char *msg, size_t msglen, const char *canonicalIP,
        int port, int flag);

ssize_t msg_recv(int sd, char *msg, size_t msglen, char *canonicalIP,
        size_t iplen, int *port);

#endif
