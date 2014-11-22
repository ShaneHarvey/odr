#ifndef ODR_H
#define ODR_H
#include "common.h"
#include "get_hw_addrs.h"
#include <sys/select.h>
#include <time.h>
#include <net/ethernet.h>
#include <sys/stat.h>

/* Used as the ethernet frame type */
#define ETH_P_ODR 0xF31F

#define ODR_MIN_STALE 0.0
#define ODR_MAX_STALE 86400.0

#define ODR_MAX_DATA 1470
#define ODR_MIN_FRAME 34

struct route_entry {
    int complete;              /* true if this route entry is complete */
    struct in_addr dstip;      /* ‘canonical’ IP address of the destination */
    unsigned char nxtmac[IFHWADDRLEN];/* MAC address of next-hop node */
    unsigned char outmac[IFHWADDRLEN];/* MAC address of outgoing if index */
    int if_index;              /* Outgoing interface index */
    int numhops;               /* Number of hops to the destination */
    uint64_t ts;               /* Timestamp of the entry */
    struct msg_node *head;     /* Head of a linked list of msgs */
    struct route_entry *next;  /* Next route in the routing table */
};

/* This is to check if we have seen this RREQ already */
struct bid_node {
    struct in_addr srcip;     /* ‘canonical’ IP address of the source node */
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
    struct in_addr srcip;     /* ‘canonical’ IP address of the destination */
    int32_t srcport;          /* Source port -- assigned by ODR */
    struct in_addr dstip;     /* ‘canonical’ IP address of the destination */
    int32_t dstport;          /* Destination port -- read from UNIX socket */
    int32_t numhops;          /* Hop count (incremented by 1 at each hop) */
    int32_t broadcastid;      /* Broadcast ID of RREQ */
    uint32_t dlen;            /* Number of bytes in the data payload */
    char data[ODR_MAX_DATA];  /* Buffer to hold data */
} __attribute__((packed));

struct msg_node {
    struct odr_msg *msg;       /* Buffered ODR payload message */
    struct msg_node *next;     /* Next msg_node that needs to be sent */
};

struct port_node {
    char permanent;            /* True if this entry is permanent */
    int port;                  /* Port for this UNIX address */
    struct sockaddr_un unaddr; /* The UNIX sockaddr_un */
    uint64_t ts;               /* Timestamp of the entry */
    struct port_node *next;    /* Next port_node in the table */
};

void run_odr(void);
int process_rreq(struct odr_msg *rreq, int srcindex, unsigned char *srcmac);
int process_rrep(struct odr_msg *rrep, int srcindex);
int process_data(struct odr_msg *data, int srcindex);

int send_rrep(struct odr_msg *rreq, struct route_entry *route,
        int32_t hops_to_dst);

int broadcast_rreq(struct odr_msg *rreq, int src_ifindex);

int send_frame(struct odr_msg *payload, unsigned char *dst_hwaddr,
        unsigned char *src_hwaddr, int ifi_index);
ssize_t recv_frame(struct ethhdr *eh, struct odr_msg *recvmsg,
        struct sockaddr_ll *src);
uint64_t usec_ts(void);
int samemac(unsigned char *mac1, unsigned char *mac2);
void hton_msg(struct odr_msg *msg);
void ntoh_msg(struct odr_msg *msg);
/*********************** BEGIN routing table functions ************************/
int route_add_complete(unsigned char *nxtmac, struct in_addr dstip, int ifindex,
        int numhops);
void route_entry_update(struct route_entry *r, unsigned char *nxtmac,
        unsigned char *outmac, int if_index, int numhops);
void route_remove(struct in_addr dest);
struct route_entry *route_lookup(struct in_addr dest);
void route_cleanup(void);
void route_free(void);
/************************ END routing table functions *************************/

/************************* BEGIN Port Table functions *************************/
struct port_node *init_port_table(void);
struct port_node *port_searchbyport(int port);
struct port_node *port_searchbyaddr(struct sockaddr_un *unaddr);
int port_add(struct sockaddr_un *addr);
void port_free(void);
/************************** END Port Table functions **************************/

/********************* BEGIN Previous RREQ list functions *********************/
struct bid_node *bid_lookup(struct in_addr src);
int bid_add(struct odr_msg *rreq);
void bid_free(void);
int ignore_rreq(struct odr_msg *rreq);
/********************* END Previous RREQ list functions ***********************/

/* Signal handling for cleanup */
static void cleanup(int signum);
static void set_sig_cleanup(void);

#endif
