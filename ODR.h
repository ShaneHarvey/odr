#ifndef ODR_H
#define ODR_H
#include "common.h"
#include "get_hw_addrs.h"
#include <sys/select.h>

/* Used as the ethernet frame type */
#define ODR_PROTOCOL 0xF31F

#define ODR_MIN_STALE 0.0
#define ODR_MAX_STALE 86400.0

/* TODO: What is the real max? */
#define ODR_MAX_DATA 256

struct route {
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    char nxthop[IFHWADDRLEN];  /* MAC address of next-hop node */
    int if_index;              /* Outgoing interface index */
    int numhops;               /* Number of hops to the destination */
    uint64_t ts;               /* Timestamp of the entry */
    struct msg_node *head;     /* Head of a linked list of msgs */
    struct route *next;        /* Next route in the routing table */
    struct route *prev;        /* Previous route in the routing table */
};

/* ODR protocol message -- data payload of ethernet frames */
#define ODR_RREQ 0
#define ODR_RREP 1
#define ODR_DATA 2

#define ODR_FORCE_RREQ 0x01
#define ODR_RREP_SENT  0x02

struct odr_msg {
    char type;                 /* ODR_RREQ, ODR_RREP, or ODR_DATA  */
    char flags;                /* Bitwise OR of ODR_FORCE_RREQ, ODR_RREP_SENT */
    struct in_addr srcip;      /* ‘canonical’ IP address of the destination */
    int32_t srcport;           /* Source port -- assigned by ODR */
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    int32_t dstport;           /* Destination port -- read from UNIX socket */
    int32_t numhops;           /* Hop count (incremented by 1 at each hop) */
    int32_t dlen;              /* Number of bytes in the data payload */
    char data[ODR_MAX_DATA];   /* Buffer to hold data */
};

struct msg_node {
    struct odr_msg *msg;       /* Buffered ODR payload message */
    struct msg_node *next;     /* Next msg_node that needs to be sent */
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
