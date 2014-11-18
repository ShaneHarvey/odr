#include "api.h"

ssize_t msg_send(int sd, char *msg, const char *canonicalIP, int port,
        int flag) {
    errno = ENOSYS;
    return -1;
}

ssize_t msg_recv(int sd, char *msg, char *canonicalIP, int *port) {
    errno = ENOSYS;
    return -1;
}
