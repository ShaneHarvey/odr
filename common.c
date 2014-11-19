#include "common.h"

int copyhostbyaddr(struct in_addr *ip, char *host, size_t hostlen) {
    struct hostent *he;

    if((he = gethostbyaddr(ip, sizeof(struct in_addr), AF_INET)) == NULL) {
        error("gethostbyaddr failed: %s\n", hstrerror(h_errno));
        return 0;
    }
    strncpy(host, he->h_name, hostlen);
    host[hostlen] = '\0';
    return 1;
}

int gethostbystr(char *canonicalIP, char *host, size_t hostlen) {
    int rv;
    struct in_addr ip;

    if(host == NULL) {
        error("host can not be NULL.\n");
        return 0;
    }
    /* Convert IP from presentation to network */
    if((rv = inet_pton(AF_INET, canonicalIP, &ip)) <= 0) {
        /* inet_pton failed */
        error("inet_pton failed: %s\n", strerror(errno));
        return 0;
    } else if(rv == 0) {
        /* canonicalIP is invalid */
        error("inet_pton failed: canonicalIP is an invalid IPv4 address.\n");
        return 0;
    }

    return copyhostbyaddr(&ip, host, hostlen);
}
