#include "common.h"

/*
 * Determine the hostname corresponding to the canonicalIP and store it into
 * host.
 * @param canonicalIP IP to lookup eg "130.245.156.22"
 * @param host        Buffer to store the hostname
 * @param hostlen     Size of the the host buffer
 * @return 1 if succeeded, 0 if failed
 */
int gethostbystr(char *canonicalIP, char *host, size_t hostlen) {
    struct hostent *he = NULL;
    struct in_addr ipv4addr;

    if(inet_aton(canonicalIP, &ipv4addr) == 0) {
        error("inet_aton failed: %s\n", strerror(errno));
        return 0;
    }

    he = gethostbyaddr(&ipv4addr, sizeof(struct in_addr), AF_INET);
    if(he == NULL) {
        error("gethostbyaddr: %s\n", hstrerror(h_errno));
        return 0;
    }
    strncpy(host, he->h_name, hostlen);
    host[hostlen - 1] = '\0';
    return 1;
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
