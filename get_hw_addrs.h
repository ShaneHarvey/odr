#ifndef HW_ADDRS_H
#define HW_ADDRS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>      /* error numbers */
#include <sys/ioctl.h>  /* ioctls */
#include <net/if.h>     /* generic interface structures */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "debug.h"

/* same as IFNAMSIZ    in <net/if.h> */
#define IF_NAME 16
/* same as IFHWADDRLEN in <net/if.h> */
#define IF_HADDR 6
/* hwa_addr is an alias */
#define IP_ALIAS 1

struct hwa_info {
    char    if_name[IF_NAME];   /* interface name, null terminated */
    char    if_haddr[IF_HADDR]; /* hardware address */
    int     if_index;           /* interface index */
    short   ip_alias;           /* 1 if hwa_addr is an alias IP address */
    struct  sockaddr *ip_addr;  /* IP address */
    struct  hwa_info *hwa_next; /* next of these structures */
};


/* function prototypes */
struct hwa_info *get_hw_addrs();
void free_hwa_info(struct hwa_info *);

#endif
