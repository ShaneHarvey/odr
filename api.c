#include "api.h"

ssize_t msg_send(int sd, char *msg, const char *canonicalIP, int port,
        int flag) {
    errno = ENOSYS;
    return -1;
}

ssize_t msg_recv(int sd, char *msg, char *canonicalIP, int *port) {
    int nread, offset;
    struct in_addr ip;
    char recvbuf[sizeof(int) + sizeof(unsigned long) + MAX_MSGLEN];

    if((nread = recv(sd, recvbuf, sizeof(recvbuf), 0)) < 0) {
        return -1;
    }
    /*TODO: Will port be Host or Network byte order?
     *TODO: Will IP be presentation or network?
     * Right now i assume that ODR will write a message like so:
     *  integer port -- sizeof(int) bytes
     *  unsigned long IP address -- sizeof(unsigned long) bytes
     *  message -- nread - sizeof(int) - sizeof(unsigned long) bytes
     */
    /* If there was no data then return error */
    if(nread <= sizeof(int) + sizeof(unsigned long)) {
        error("Invalid message from ODR: too short\n");
        errno = EBADMSG;
        return -1;
    }
    /* parse port */
    memcpy(port, recvbuf, sizeof(int));
    /* parse IP */
    offset = sizeof(int);
    memcpy(&ip.s_addr, recvbuf + offset, sizeof(unsigned long));
    if(inet_ntop(AF_INET, &ip, canonicalIP, MAX_IPLEN) == NULL) {
        return -1;
    }
    /* Parse message */
    offset += sizeof(unsigned long);
    memcpy(msg, recvbuf + offset, nread - offset);
    msg[nread - offset] = '\0';
    return nread - offset;
}
