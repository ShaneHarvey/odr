#include "ODR.h"

int main(int argc, char **argv) {
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
        error("Invalid staleness argument: %s\n", strerror(errno));
        return EXIT_FAILURE;
    } else if (endptr == argv[1] || *endptr != '\0'){
        error("Invalid staleness: must be a double\n");
        return EXIT_FAILURE;
    }
    info("staleness = %f seconds.\n", staleness);

    return EXIT_SUCCESS;
}
