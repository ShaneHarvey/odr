#ifndef ODR_H
#define ODR_H
#include "common.h"
#include "get_hw_addrs.h"
#include <sys/select.h>
#include <time.h>
#include <net/ethernet.h>

/* Used as the ethernet frame type */
#define ETH_P_ODR 0xF31F

#define ODR_MIN_STALE 0.0
#define ODR_MAX_STALE 86400.0

#define ODR_MAX_DATA 1470

struct route_entry {
    int complete;              /* true if this route entry is complete */
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    char nxtmac[IFHWADDRLEN];  /* MAC address of next-hop node */
    char outmac[IFHWADDRLEN];  /* MAC address of outgoing interface index */
    int if_index;              /* Outgoing interface index */
    int numhops;               /* Number of hops to the destination */
    uint64_t ts;               /* Timestamp of the entry */
    struct msg_node *head;     /* Head of a linked list of msgs */
    struct route_entry *next;  /* Next route in the routing table */
    struct route_entry *prev;  /* Previous route in the routing table */
};

/* This is to check if we have seen this RREQ already */
struct bid_node {
    struct in_addr nodeip;     /* ‘canonical’ IP address of the node */
    int32_t broadcastid;       /* Highest ID seen from broadcast from node */
    int numhops;               /* numhops sent on the last RREP */
    struct bid_node* next;     /* Next bid_node in the list */
};

/* ODR protocol message -- data payload of ethernet frames */
#define ODR_RREQ 0
#define ODR_RREP 1
#define ODR_DATA 2
#define ODR_FORCE_RREQ 0x01
#define ODR_RREP_SENT  0x02

/* Size of a odr_msg */
#define ODR_MSG_SIZE(msgptr) (30 + (msgptr)->dlen)

struct odr_msg {
    char type;                /* ODR_RREQ, ODR_RREP, or ODR_DATA  */
    char flags;               /* Bitwise OR of ODR_FORCE_RREQ, ODR_RREP_SENT */
    struct in_addr srcip;      /* ‘canonical’ IP address of the destination */
    int32_t srcport;           /* Source port -- assigned by ODR */
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    int32_t dstport;           /* Destination port -- read from UNIX socket */
    int32_t numhops;           /* Hop count (incremented by 1 at each hop) */
    int32_t broadcastid;       /* Broadcast ID of RREQ */
    uint32_t dlen;             /* Number of bytes in the data payload */
    char data[ODR_MAX_DATA];   /* Buffer to hold data */
} __attribute__((packed));

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

void run_odr(void);

int process_rreq(struct odr_msg *rreq, struct sockaddr_ll *llsrc,
        socklen_t srclen);

int process_rrep(struct odr_msg *rrep, struct sockaddr_ll *llsrc,
        socklen_t srclen);

int process_data(struct odr_msg *data, struct sockaddr_ll *llsrc,
        socklen_t srclen);

int send_rrep(struct odr_msg *rreq, struct route_entry *route,
        int32_t hops_to_dst);

int broadcast_rreq(struct odr_msg *rreq, int src_ifindex);

int send_frame(void *frame_data, int size, char *dst_hwaddr, char *src_hwaddr,
        int ifi_index);

int duplicate_rreq(struct odr_msg *rreq);

struct route_entry *route_lookup(struct in_addr dest);

void cleanup_stale(struct route_entry *routingTable);

struct bid_node *bid_lookup(struct in_addr src);

void bid_add(struct odr_msg *rreq);

#endif
