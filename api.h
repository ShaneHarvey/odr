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

ssize_t msg_send(int sd, char *msg, size_t msglen, const char *canonicalIP,
        int port, int flag);

ssize_t msg_recv(int sd, char *msg, size_t msglen, char *canonicalIP,
        size_t iplen, int *port);

#endif
