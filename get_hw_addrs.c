#include "get_hw_addrs.h"


struct hwa_info *get_hw_addrs(void) {
    struct hwa_info    *hwa, *hwahead, **hwapnext;
    int       sockfd, len, lastlen, alias, nInterfaces, i;
    char   *buf, lastname[IFNAMSIZ], *cptr;
    struct ifconf    ifc;
    struct ifreq    *ifr, *item, ifrcopy;
    struct sockaddr    *sinptr;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return NULL;
    }

    lastlen = 0;
    len = 100 * sizeof(struct ifreq);    /* initial buffer size guess */
    for ( ; ; ) {
        if((buf = malloc(len)) == NULL) {
            goto CLOSE_SOCK;
        }
        ifc.ifc_len = len;
        ifc.ifc_buf = buf;
        if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
            if (errno != EINVAL || lastlen != 0) {
                error("ioctl error: %s\n", strerror(errno));
                goto FREE_BUF;
            }
        } else {
            if (ifc.ifc_len == lastlen) {
                break;        /* success, len has not changed */
            }
            lastlen = ifc.ifc_len;
        }
        len += 10 * sizeof(struct ifreq);    /* increment */
        free(buf);
    }

    hwahead = NULL;
    hwapnext = &hwahead;
    lastname[0] = 0;
    
    ifr = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
    for(i = 0; i < nInterfaces; i++)  {
        item = &ifr[i];
         alias = 0; 
        if((hwa = calloc(1, sizeof(struct hwa_info))) == NULL) {
            goto FREE_BUF;
        }

        memcpy(hwa->if_name, item->ifr_name, IFNAMSIZ);        /* interface name */
        hwa->if_name[IFNAMSIZ-1] = '\0';
        /* start to check if alias address */
        if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
            *cptr = 0;        /* replace colon will null */
        if (strncmp(lastname, item->ifr_name, IFNAMSIZ) == 0) {
            alias = IP_ALIAS;
        }
        memcpy(lastname, item->ifr_name, IFNAMSIZ);
        ifrcopy = *item;
        *hwapnext = hwa;        /* prev points to this new one */
        hwapnext = &hwa->hwa_next;    /* pointer to next one goes here */

        hwa->ip_alias = alias;        /* alias IP address flag: 0 if no; 1 if yes */
        sinptr = &item->ifr_addr;
        if((hwa->ip_addr = calloc(1, sizeof(struct sockaddr))) == NULL) {
            goto FREE_BUF;
        }
        memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));    /* IP address */
        /* get hw address */
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0) {
            error("ioctl SIOCGIFHWADDR failed: %s", strerror(errno));
        }
        memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IFHWADDRLEN);
        /* get interface index */
        if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0) {
            error("ioctl SIOCGIFINDEX failed: %s", strerror(errno));
        }
        memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
    }
    free(buf);
    return(hwahead);    /* pointer to first structure in linked list */
FREE_BUF:
    free(buf);
CLOSE_SOCK:
    close(sockfd);
    return NULL;
}

void free_hwa_info(struct hwa_info *hwahead) {
    struct hwa_info    *hwa, *hwanext;

    for (hwa = hwahead; hwa != NULL; hwa = hwanext) {
        free(hwa->ip_addr);
        hwanext = hwa->hwa_next;    /* can't fetch hwa_next after free() */
        free(hwa);            /* the hwa_info{} itself */
    }
}
