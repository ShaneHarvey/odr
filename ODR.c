#include "ODR.h"

static char myhost[HOST_NAME_MAX];

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

int main(int argc, char **argv) {
    int unixsock, rawsock;
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
        error("Arg staleness: %s\n", strerror(errno));
        return EXIT_FAILURE;
    } else if (endptr == argv[1] || *endptr != '\0') {
        error("Arg staleness: must be a double\n");
        return EXIT_FAILURE;
    } else if(staleness < 0.0 || staleness > 3600.0) {
        error("Arg staleness: must be between 0.0-3600.0 seconds\n");
        return EXIT_FAILURE;
    } else {
        info("staleness = %f seconds.\n", staleness);
    }

    /* Create raw socket to receive only our ODR protocol */
    if((rawsock = socket(AF_PACKET, SOCK_RAW, htons(ODR_PROTOCOL))) < 0) {
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
    if(gethostname(myhost, sizeof(myhost)) < 0) {
        error("gethostname failed: %s\n", strerror(errno));
        goto FREE_HWA;
    } else {
        info("ODR running on node %s\n", myhost);
    }

    /* Start the ODR service */
    run_odr(unixsock, rawsock, hwahead);
FREE_HWA:
    free_hwa_info(hwahead);
CLOSE_UNIX:
    /* unlink the file */
    unlink(ODR_PATH);
    close(unixsock);
CLOSE_RAW:
    close(rawsock);
    return EXIT_FAILURE;
}

void run_odr(int unixsock, int rawsock, struct hwa_info *hwahead) {

}

