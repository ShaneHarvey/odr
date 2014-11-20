#include "ODR.h"

/* Static Globals used by ODR */

static char odrhost[HOST_NAME_MAX];   /* Hostname running ODR, eg vm2 */
static struct in_addr odraddr;        /* 'Canonical' IP running ODR   */
static uint64_t route_ttl = 1000000L; /* Route TTL in microseconds    */

static void cleanup(int signum);
static void set_sig_cleanup(void);

int main(int argc, char **argv) {
    int unixsock, packsock;
    struct sockaddr_un unaddr;
    struct hwa_info *hwahead;
    double staleness;
    char *endptr;

    if(argc != 2) {
        fprintf(stderr, "Usage:   %s staleness_in_seconds\n", argv[0]);
        fprintf(stderr, "Example: %s 2\n", argv[0]);
        fprintf(stderr, "         %s 0.2\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Parse the staleness argument */
    errno = 0;
    staleness = strtod(argv[1], &endptr);
    if(errno != 0) {
        error("staleness arg: %s\n", strerror(errno));
        return EXIT_FAILURE;
    } else if (endptr == argv[1] || *endptr != '\0') {
        error("staleness arg: must be a double\n");
        return EXIT_FAILURE;
    } else if(staleness < ODR_MIN_STALE || staleness > ODR_MAX_STALE) {
        error("staleness arg: must be between %.1f-%.1f seconds\n",
                ODR_MIN_STALE, ODR_MAX_STALE);
        return EXIT_FAILURE;
    } else {
        info("staleness = %f seconds.\n", staleness);
        route_ttl = 1000000L * staleness;
    }

    /* Create packet socket to receive only our ODR protocol */
    if((packsock = socket(AF_PACKET, SOCK_RAW, htons(ODR_PROTOCOL))) < 0) {
        error("socket failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Set SIGINT handler to socket file */
    set_sig_cleanup();

    /* Create UNIX socket */
    if((unixsock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        error("socket failed: %s\n", strerror(errno));
        goto CLOSE_RAW;
    }
    /* Bind to well known file */
    unaddr.sun_family = AF_UNIX;
    strncpy(unaddr.sun_path, ODR_PATH, sizeof(unaddr.sun_path) - 1);
    /* unlink the file */
    unlink(ODR_PATH);
    if(bind(unixsock, (struct sockaddr*)&unaddr, sizeof(unaddr)) < 0) {
        error("bind failed: %s\n", strerror(errno));
        goto CLOSE_UNIX;
    }

    /* Find our interfaces */
    if((hwahead = get_hw_addrs()) == NULL) {
        error("Failed to get hardware addresses\n");
        goto CLOSE_UNIX;
    }

    /* Lookup our hostname */
    if(gethostname(odrhost, sizeof(odrhost)) < 0) {
        error("gethostname failed: %s\n", strerror(errno));
        goto FREE_HWA;
    } else {
        info("ODR running on node %s\n", odrhost);
    }

    /* Lookup our ip address */
    if(getipbyhost(odrhost, &odraddr)) {
        debug("ODR running on IP %s\n", inet_ntoa(odraddr));
    } else {
        goto FREE_HWA;
    }

    /* Start the ODR service */
    run_odr(unixsock, packsock, hwahead);
FREE_HWA:
    free_hwa_info(hwahead);
CLOSE_UNIX:
    /* unlink the file */
    unlink(ODR_PATH);
    close(unixsock);
CLOSE_RAW:
    close(packsock);
    return EXIT_FAILURE;
}

void run_odr(int unixsock, int packsock, struct hwa_info *hwahead) {
    int maxfd, nread;
    fd_set rset;
    struct route *routingTable = NULL;

    maxfd = unixsock > packsock ? unixsock + 1 : packsock + 1;
    /* Select on the two sockets forever */
    while(1) {
        FD_ZERO(&rset);
        FD_SET(unixsock, &rset);
        FD_SET(packsock, &rset);
        if(select(maxfd, &rset, NULL, NULL, NULL) < 0) {
            error("select failed: %s\n", strerror(errno));
            return;
        }

        /* UNIX socket is readable */
        if(FD_ISSET(unixsock, &rset)) {
            struct sockaddr_un unaddr;
            socklen_t addrlen;
            struct api_msg recvmsg;

            if((nread = recvfrom(unixsock, &recvmsg, sizeof(recvmsg), 0,
                    (struct sockaddr *)&unaddr, &addrlen)) < 0) {
                error("UNIX socket recvfrom failed: %s\n", strerror(errno));
                return;
            } else if(nread < MIN_API_MSG) {
                /* Ignore messages that are too short */
                warn("Ignoring short message from UNIX socket\n");
            } else {
                /* valid API message received */
                info("ODR received valid message from UNIX socket\n");
                cleanup_stale(routingTable);
            }
        }

        /* Packet socket is readable */
        if(FD_ISSET(packsock, &rset)) {
            struct odr_msg recvmsg;
            struct sockaddr_ll llsrc;
            socklen_t srclen;

            if((nread = recvfrom(unixsock, &recvmsg, sizeof(recvmsg), 0,
                    (struct sockaddr *)&llsrc, &srclen)) < 0) {
                error("packet socket recv failed: %s\n", strerror(errno));
                return;
            } else {
                /* valid API message received */
                info("ODR received valid packet from packet socket\n");
                /* Update route table */
                cleanup_stale(routingTable);
                /* if FORCE_RREQ then remove_route(dest ip) */
                /* add_route to source MAC <-------Update if shorter numhops OR same hop but
                diff ifindex or MAC */
                /* proces ODR message */
                switch(recvmsg.type) {
                    case ODR_RREQ:
                        process_rreq(&recvmsg, &llsrc, srclen);
                        break;
                    case ODR_RREP:
                        process_rrep(&recvmsg, &llsrc, srclen);
                        break;
                    case ODR_DATA:
                        process_data(&recvmsg, &llsrc, srclen);
                        break;
                    default:
                        warn("Invalid message type %d\n", recvmsg.type);
                }
            }
        }
    }
}

int process_rreq(struct odr_msg *rreq, struct sockaddr_ll *llsrc,
        socklen_t srclen) {
    return 0;
}

int process_rrep(struct odr_msg *rrep, struct sockaddr_ll *llsrc,
        socklen_t srclen) {
    return 0;
}

int process_data(struct odr_msg *data, struct sockaddr_ll *llsrc,
        socklen_t srclen) {
    return 0;
}

void cleanup_stale(struct route *routingTable) {
    if(routingTable != NULL) {
        /* Cleanup */
    }
}

static void cleanup(int signum) {
    /* remove the UNIX socket file */
    unlink(ODR_PATH);
    /* 128+n Fatal error signal "n" is the standard Linux exit code */
    exit(128 + signum);
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
