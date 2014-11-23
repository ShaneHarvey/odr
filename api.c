#include "api.h"

ssize_t msg_send(int sd, char *msg, size_t msglen, const char *canonicalIP,
        int port, int flag) {
    int rv, sendlen;
    struct sockaddr_un odr_addr;
    struct api_msg sendmsg;
    int nsent;

    memset(&odr_addr, 0, sizeof(struct sockaddr_un));
    memset(&sendmsg, 0, sizeof(struct api_msg));

    if(sd < 0 || msg == NULL || canonicalIP == NULL || port < 0 || flag < 0) {
        errno = EINVAL;
        return -1;
    }
    if(msglen > MAX_MSGLEN) {
        /* message is too long for API */
        errno = EMSGSIZE;
        return -1;
    }
    /* Convert string IP to network */
    if((rv = inet_pton(AF_INET, canonicalIP, &sendmsg.ip)) <= 0) {
        /* inet_pton failed */
        return -1;
    } else if(rv == 0) {
        /* canonicalIP is invalid */
        errno = EINVAL;
        return -1;
    }
    /* Copy port */
    sendmsg.port = port;
    /* Copy flag */
    sendmsg.flag = flag;
    /* Copy message */
    memcpy(sendmsg.msg, msg, msglen);
    /* Find out the length to send */
    sendlen = MIN_API_MSG + msglen;

    /* SEND to ODR well known path */
    odr_addr.sun_family = AF_UNIX;
    strncpy(odr_addr.sun_path, ODR_PATH, sizeof(odr_addr.sun_path) - 1);

    if((nsent = sendto(sd, &sendmsg, sendlen, 0, (struct sockaddr*)&odr_addr,
            sizeof(odr_addr))) < 0) {
        /* sendto failed to write all the data */
        error("API failed to sendto ODR UNIX socket file: %s\n",
                strerror(errno));
        return -1;
    } else if(nsent < sendlen) {
        error("Only %d bytes out of %d were sent\n", nsent, sendlen);
        return -1;
    } else {
        /* return data length to application */
        return msglen;
    }
}

ssize_t msg_recv(int sd, char *msg, size_t msglen, char *canonicalIP,
        size_t iplen, int *port) {
    int recvlen, rmsglen, minlen;
    struct api_msg recvmsg;
    memset(&recvmsg, 0, sizeof(struct api_msg));

    if(sd < 0 || msg == NULL || canonicalIP == NULL || port == NULL) {
        errno = EINVAL;
        return -1;
    }

    if((recvlen = recv(sd, &recvmsg, sizeof(recvmsg), 0)) < 0) {
        /* recv(2) failed */
        return recvlen;
    } else if(recvlen < MIN_API_MSG) {
        /* The message was too small to be an ODR message */
        error("Invalid message from ODR: too short\n");
        errno = EBADMSG;
        return -1;
    }

    /* parse IP */
    sprintf(canonicalIP, "%s", inet_ntoa(recvmsg.ip));
    /* Copy port */
    *port = recvmsg.port;
    /* Copy message and null-terminate */
    rmsglen = recvlen - MIN_API_MSG;
    minlen = rmsglen < msglen? rmsglen : msglen - 1;
    memcpy(msg, recvmsg.msg, minlen);
    msg[minlen] = '\0';
    return minlen;
}
