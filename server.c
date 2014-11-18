#include "server.h"

static int running = 1;

static void sigint(int signum, siginfo_t *siginfo, void *context) {
    running = 0;
}

int main(int argc, char *argv[]) {
    int unix_socket;
    struct sockaddr_un addr;

    struct sigaction sigac_int;
    /* Zero out memory */
    memset(&sigac_int, 0, sizeof(sigac_int));
    /* Set values */
    sigac_int.sa_sigaction = &sigint;
    sigac_int.sa_flags = SA_SIGINFO;
    /* Set the sigactions */
    if(sigaction(SIGINT, &sigac_int, NULL) < 0) {
        error("sigaction failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    /* Create UNIX socket */
    unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0);

    /* Bind to well known file */
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SERVER_PATH, sizeof(addr.sun_path) - 1);
    /* unlink the file */
    unlink(addr.sun_path);
    if(bind(unix_socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        /* Run the time server */
        run_time_server(unix_socket);
    } else {
        /* bind failed */
        error("bind failed: %s\n", strerror(errno));
    }
    /* unlink the file */
    unlink(addr.sun_path);
    close(unix_socket);
    return EXIT_SUCCESS;
}

/*
 * Infinite loop of msg_recv and msg_send
 */
void run_time_server(int unix_socket) {
    int rv, port;
    time_t ticks;
    char recvbuf[MAX_MSGLEN], sendbuf[MAX_MSGLEN], ip[MAX_IPLEN], *timestr;

    running = 1;
    while(running) {
        /* msg_recv */
        if((rv = msg_recv(unix_socket, recvbuf, ip, &port)) < 0) {
            error("msg_recv: returned %d, errno %d: %s\n", rv, errno,
                    strerror(errno));
            break;
        }
        /* Write time to the client */
        if ((ticks = time(NULL)) == ((time_t) - 1)) {
            error("time failed: %s\n", strerror(errno));
            break;
        }
        if ((timestr = ctime(&ticks)) == NULL) {
            error("ctime failed: %s\n", strerror(errno));
            break;
        }
        snprintf(sendbuf, sizeof(sendbuf), "%.24s\r\n", timestr);
        /* Send buff to client using msg_send */
        if ((rv = msg_send(unix_socket, sendbuf, ip, port, 0)) < 0) {
            error("msg_send: returned %d, errno %d: %s\n", rv, errno,
                    strerror(errno));
            break;
        }
    }
}
