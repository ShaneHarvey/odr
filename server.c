#include "server.h"

static void cleanup(int signum);
static void set_sig_cleanup(void);

int main(int argc, char *argv[]) {
    int unix_socket;
    struct sockaddr_un addr;

    set_sig_cleanup();
    /* Create UNIX socket */
    if((unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        error("socket failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

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
    return EXIT_FAILURE;
}

/*
 * Infinite loop of msg_recv and msg_send
 */
void run_time_server(int unix_socket) {
    int rv, port;
    time_t ticks;
    char myhost[HOST_NAME_MAX], chost[HOST_NAME_MAX], recvbuf[MAX_MSGLEN],
            sendbuf[MAX_MSGLEN], ip[MAX_IPLEN], *timestr;

    /* Lookup our hostname */
    if(gethostname(myhost, sizeof(myhost)) < 0) {
        error("gethostname failed: %s\n", strerror(errno));
        return;
    }
    info("server running at node %s\n", myhost);
    while(1) {
        /* msg_recv */
        if((rv = msg_recv(unix_socket, recvbuf, sizeof(recvbuf), ip,
                sizeof(ip), &port)) < 0) {
            error("msg_recv: returned %d, errno %d: %s\n", rv, errno,
                    strerror(errno));
            break;
        }
        /* Determine client's hostname */
        if(!gethostbystr(ip, chost, sizeof(chost))) {
            break;
        }
        info("server at node %s responding to request from %s\n", myhost,
                chost);
        /* Construct a timestamp */
        if ((ticks = time(NULL)) == ((time_t) - 1)) {
            error("time failed: %s\n", strerror(errno));
            break;
        }
        if ((timestr = ctime(&ticks)) == NULL) {
            error("ctime failed: %s\n", strerror(errno));
            break;
        }
        snprintf(sendbuf, sizeof(sendbuf), "%.24s", timestr);
        /* Send buff to client using msg_send */
        if ((rv = msg_send(unix_socket, sendbuf, strlen(sendbuf), ip, port,
                0)) < 0) {
            error("msg_send: returned %d, errno %d: %s\n", rv, errno,
                    strerror(errno));
            break;
        }
    }
}

static void cleanup(int signum) {
    /* remove the UNIX socket file */
    unlink(SERVER_PATH);
    /* 128+n Fatal error signal "n" is the standard Linux exit code */
    _exit(128 + signum);
}

static void set_sig_cleanup(void) {
    struct sigaction sigac_int;
    /* Zero out memory */
    memset(&sigac_int, 0, sizeof(sigac_int));
    /* Set values */
    sigac_int.sa_handler = &cleanup;
    /* Set the sigaction */
    if(sigaction(SIGINT, &sigac_int, NULL) < 0) {
        error("sigaction failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
