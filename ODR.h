#ifndef ODR_H
#define ODR_H
#include "common.h"
#include "get_hw_addrs.h"
#include <sys/select.h>

#define ODR_PROTOCOL 62239
/* TODO: What is the real max? */
#define ODR_MAX_DATA 256

struct route {
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    char nxthop[IFHWADDRLEN];  /* MAC address of next-hop node */
    int if_index;              /* Outgoing interface index */
    int numhops;               /* Number of hops to the destination */
    uint64_t ts;               /* Timestamp of the entry */
    struct odr_msg *msg_head;  /* Head of a linked list of msgs */
    struct route *next;        /* Next route in the routing table */
    struct route *prev;        /* Previous route in the routing table */
};

struct odr_msg {
    int dstport;               /* Destination port -- read from UNIX socket */
    int srcport;               /* Source port -- assigned by ODR */
    size_t dlen;               /* Number of bytes in the data payload */
    char data[ODR_MAX_DATA];   /* Buffer to hold data */
    struct odr_msg *next;      /* Next odr_msg that needs to be sent */
};

struct odr_port {
    int port;
    struct sockaddr_un unaddr; /* The UNIX sockaddr_un */
    socklen_t addrlen;         /* Length of unaddr */
    uint64_t ts;               /* Timestamp of the entry */
    struct odr_port *next;     /* Next odr_port in the table */
};

void run_odr(int unixsock, int rawsock, struct hwa_info *hwahead);

#endif
