#include "common.h"

/*
 * Determine the hostname corresponding to the in_addr and store it into host.
 * @param ip          in_addr address to lookup
 * @param host        Buffer to store the hostname
 * @param hostlen     Size of the the host buffer
 * @return 1 if succeeded, 0 if failed
 */
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

/*
 * Determine the hostname corresponding to the canonicalIP and store it into
 * host.
 * @param canonicalIP IP to lookup eg "130.245.156.22"
 * @param host        Buffer to store the hostname
 * @param hostlen     Size of the the host buffer
 * @return 1 if succeeded, 0 if failed
 */
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

/*
 * Determine the ip corresponding to the hostname
 *
 * @param hostname    string hostname
 * @param hostip      Pointer to an in_addr to store the IP of hostname
 * @return 1 if succeeded, 0 if failed
 */
int getipbyhost(char *hostname, struct in_addr *hostip) {
    struct hostent *he;
    struct in_addr **addr_list;

    if(hostname == NULL || hostip == NULL) {
        return 0;
    }
    if ((he = gethostbyname(hostname)) == NULL) {
        error("gethostbyname failed: %s\n", hstrerror(h_errno));
        return 0;
    }
    addr_list = (struct in_addr **)he->h_addr_list;
    *hostip = *addr_list[0];
    return 1;
}
