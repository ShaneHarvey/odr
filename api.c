#include "api.h"

ssize_t msg_send(int sd, char *msg, const char *canonicalIP, int port,
        int flag) {
    int size, rv;
    struct sockaddr_un odr_addr;
    struct in_addr ip;
    char sendbuf[sizeof(int) + sizeof(struct in_addr) + MAX_MSGLEN];

    if(sd < 0 || msg == NULL || canonicalIP == NULL || port < 0 || flag < 0) {
        errno = EINVAL;
        return -1;
    }
    /* Convert string IP to network */
    if((rv = inet_pton(AF_INET, canonicalIP, &ip)) <= 0) {
        /* inet_pton failed */
        return -1;
    } else if(rv == 0) {
        /* canonicalIP is invalid */
        errno = EINVAL;
        return -1;
    }
    size = 0;
    /* Copy address */
    memcpy(sendbuf + size, &ip, sizeof(struct in_addr));
    size += sizeof(struct in_addr);
    /* Copy port */
    memcpy(sendbuf + size, &port, sizeof(int));
    size += sizeof(int);
    /* Copy flag */
    memcpy(sendbuf + size, &flag, sizeof(int));
    size += sizeof(int);
    /* Copy message */
    strcpy(sendbuf + size, msg);
    size += strlen(msg);

    /* SEND to ODR well known path */
    odr_addr.sun_family = AF_UNIX;
    strncpy(odr_addr.sun_path, ODR_PATH, sizeof(odr_addr.sun_path) - 1);

    if(sendto(sd, sendbuf, size, 0, (struct sockaddr*)&odr_addr,
            sizeof(odr_addr)) < size) {
        /* sendto failed to write all the data */
        return -1;
    }
    return strlen(msg);
}

ssize_t msg_recv(int sd, char *msg, char *canonicalIP, int *port) {
    int nread, offset;
    struct in_addr ip;
    char recvbuf[sizeof(int) + sizeof(struct in_addr) + MAX_MSGLEN];

    if(sd < 0 || msg == NULL || canonicalIP == NULL || port == NULL) {
        errno = EINVAL;
        return -1;
    }

    if((nread = recv(sd, recvbuf, sizeof(recvbuf), 0)) < 0) {
        return -1;
    }
    /*TODO: Will port/IP be Host or Network byte order?
     * Right now i assume that ODR will write a message like so:
     *  struct in_addr IP address -- sizeof(struct in_addr) bytes
     *  integer port -- sizeof(int) bytes
     *  message -- nread - sizeof(int) - sizeof(struct in_addr) bytes
     */
    /* If there was no data then return error */
    if(nread <= sizeof(int) + sizeof(struct in_addr)) {
        error("Invalid message from ODR: too short\n");
        errno = EBADMSG;
        return -1;
    }
    offset = 0;
    /* parse IP */
    memcpy(&ip, recvbuf, sizeof(struct in_addr));
    offset += sizeof(struct in_addr);
    if(inet_ntop(AF_INET, &ip, canonicalIP, MAX_IPLEN) == NULL) {
        return -1;
    }
    /* parse port */
    memcpy(port, recvbuf + offset, sizeof(int));
    offset += sizeof(int);
    /* Parse message */
    memcpy(msg, recvbuf + offset, nread - offset);
    msg[nread - offset] = '\0';
    return nread - offset;
}
