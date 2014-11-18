#ifndef API_H
#define API_H
#include <sys/types.h>

#define MAX_MSGLEN 1024
#define MAX_IPLEN 32

ssize_t msg_send(int sd, char *msg, const char *canonicalIP, int port,
        int flag);

ssize_t msg_recv(int sd, char *msg, char *canonicalIP, int *port);

#endif
