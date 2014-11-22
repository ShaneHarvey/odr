#include "ODR.h"

/* Static Globals used by ODR, LOTS of em :) */
static char odrhost[HOST_NAME_MAX];   /* Hostname running ODR, eg vm2    */
static struct in_addr odrip;          /* 'Canonical' IP running ODR      */
static uint64_t route_ttl = 1000000L; /* Route TTL in microseconds       */
static int32_t broadcastid = 1;       /* Broadcast ID for next RREQ      */
static int unixsock = -1;             /* fd of UNIX domain socket        */
static int packsock = -1;             /* fd of packet socket             */
struct hwa_info *hwahead = NULL;      /* List of Hardware Addresses      */
struct bid_node *bidhead = NULL;      /* List of RREQ broadcast IDS seen */
struct port_node *porthead = NULL;    /* List of local port allocations  */
struct route_entry *routehead = NULL; /* Routing Table                   */

int main(int argc, char **argv) {
    struct sockaddr_un unaddr;
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
        error("staleness arg: %s\n", strerror(errno));
        return EXIT_FAILURE;
    } else if (endptr == argv[1] || *endptr != '\0') {
        error("staleness arg: must be a double\n");
        return EXIT_FAILURE;
    } else if(staleness < ODR_MIN_STALE || staleness > ODR_MAX_STALE) {
        error("staleness arg: must be between %.1f-%.1f seconds\n",
                ODR_MIN_STALE, ODR_MAX_STALE);
        return EXIT_FAILURE;
    } else {
        info("staleness = %f seconds.\n", staleness);
        route_ttl = (uint64_t)(1000000ULL * staleness);
    }

    /* Create packet socket to receive only our ODR protocol */
    if((packsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ODR))) < 0) {
        error("socket failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Set SIGINT handler to socket file */
    set_sig_cleanup();

    /* Create UNIX socket */
    if((unixsock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        error("socket failed: %s\n", strerror(errno));
        goto CLOSE_RAW;
    }
    /* Bind to well known file */
    unaddr.sun_family = AF_UNIX;
    strncpy(unaddr.sun_path, ODR_PATH, sizeof(unaddr.sun_path) - 1);
    /* unlink the file */
    unlink(ODR_PATH);
    if(bind(unixsock, (struct sockaddr*)&unaddr, sizeof(unaddr)) < 0) {
        error("bind failed: %s\n", strerror(errno));
        goto CLOSE_UNIX;
    }

    /* chmod the file to world writable (722) so any process can use ODR  */
    if(chmod(ODR_PATH, S_IRUSR| S_IWUSR| S_IXUSR | S_IWGRP | S_IWOTH) < 0) {
        error("chmod failed: %s\n", strerror(errno));
        goto CLOSE_UNIX;
    }

    /* Find our interfaces */
    if((hwahead = get_hw_addrs()) == NULL) {
        error("Failed to get hardware addresses\n");
        goto CLOSE_UNIX;
    }

    /* Lookup our hostname */
    if(gethostname(odrhost, sizeof(odrhost)) < 0) {
        error("gethostname failed: %s\n", strerror(errno));
        goto FREE_HWA;
    } else {
        info("ODR running on node %s\n", odrhost);
    }

    /* Lookup our ip address */
    if(getipbyhost(odrhost, &odrip)) {
        debug("ODR running on IP %s\n", inet_ntoa(odrip));
    } else {
        goto FREE_HWA;
    }
    info("ODR initial broadcast ID %d\n", broadcastid);
    /* Init the port table with local server address/port */
    if((porthead = init_port_table()) == NULL) {
        goto FREE_HWA;
    }
    /* Start the ODR service */
    run_odr();
    /* cleanup stuff if run_odr returns */
    route_free();
    port_free();
    bid_free();
FREE_HWA:
    free_hwa_info(hwahead);
CLOSE_UNIX:
    /* unlink the file */
    unlink(ODR_PATH);
    close(unixsock);
CLOSE_RAW:
    close(packsock);
    return EXIT_FAILURE;
}

/*
 * Infinte select loop on the UNIX domain socket and the packet socket.
 */
void run_odr(void) {
    int maxfd, nread;
    fd_set rset;

    maxfd = unixsock > packsock ? unixsock + 1 : packsock + 1;
    /* Select on the two sockets forever */
    while(1) {
        FD_ZERO(&rset);
        FD_SET(unixsock, &rset);
        FD_SET(packsock, &rset);
        if(select(maxfd, &rset, NULL, NULL, NULL) < 0) {
            error("select failed: %s\n", strerror(errno));
            return;
        }

        /* UNIX socket is readable */
        if(FD_ISSET(unixsock, &rset)) {
            struct sockaddr_un unaddr;
            socklen_t addrlen;
            struct api_msg recvmsg;

            if((nread = recvfrom(unixsock, &recvmsg, sizeof(recvmsg), 0,
                    (struct sockaddr *)&unaddr, &addrlen)) < 0) {
                error("UNIX socket recvfrom failed: %s\n", strerror(errno));
                return;
            } else if(nread < MIN_API_MSG) {
                /* Ignore messages that are too short */
                warn("Ignoring short message from UNIX socket\n");
            } else {
                /* valid API message received */
                info("ODR received valid message from UNIX socket\n");
                route_cleanup();
            }
        }

        /* Packet socket is readable */
        if(FD_ISSET(packsock, &rset)) {
            struct ethhdr eh;
            struct odr_msg recvmsg;
            struct sockaddr_ll llsrc;
            int updated;

            if((nread = recv_frame(&eh, &recvmsg, &llsrc) < 0)) {
                /* FAILED */
                return;
            } else if(nread < ODR_MIN_FRAME) {
                warn("Received ethernet frame too small for ODR.\n");
            } else {
                /* Received a valid ODR message */
                if(recvmsg.srcip.s_addr == odrip.s_addr) {
                    /* Received a message from this ODR */
                    info("ODR received packet from self.\n");
                } else {
                    /* valid API message received */
                    info("ODR received valid packet from packet socket\n");
                    /* Update route table */
                    route_cleanup();
                    if(recvmsg.flags & ODR_FORCE_RREQ) {
                        /* if FORCE_RREQ then remove_route(dest ip) */
                        route_remove(recvmsg.dstip);
                    }
                    /* add the route back to source with ifindex/ nxtMAC */
                    if((updated = route_add_complete(eh.h_source,
                            recvmsg.srcip, htonl(llsrc.sll_ifindex),
                            htonl(recvmsg.numhops))) < 0) {
                        /* failed */
                        return;
                    }
                    info("ODR routing table %s \n", updated? "updated": "did not update");
                    /* proces ODR message */
                    switch(recvmsg.type) {
                        case ODR_RREQ:
                            if(!process_rreq(&recvmsg, &llsrc)){
                                return;
                            }
                            break;
                        case ODR_RREP:
                            if(!process_rrep(&recvmsg, &llsrc)) {
                                return;
                            }
                            break;
                        case ODR_DATA:
                            if(!process_data(&recvmsg, &llsrc)) {
                                return;
                            }
                            break;
                        default:
                            warn("Invalid message type %d\n", recvmsg.type);
                    }
                }
            }
        }
    }
}

/*
 * @param rreq    Pointer to a valid RREQ type odr_msg
 * @param llsrc   Link layer source address of the RREQ
 *
 * @return True if succeeded, false if failed
 */
int process_rreq(struct odr_msg *rreq, struct sockaddr_ll *llsrc) {
    struct route_entry *srcroute;

    /* check if the RREQ is a duplicate */
    if(ignore_rreq(rreq)) {
        warn("duplicate RREQ ignored\n");
        return 1;
    } else {
        /* add bid entry to the broadcast id list */
        if(!bid_add(rreq)) {
            /* failed to add */
            return 0;
        }
    }

    /* Lookup the route to the source*/
    srcroute = route_lookup(rreq->srcip);

    if(rreq->dstip.s_addr == odrip.s_addr) {
        /* We are the destination, send an RREP back to the source */
        if(send_rrep(rreq, srcroute, 0) < 0) {
            return 0;
        }
    } else {
        /* We are an intermediate node */
        struct route_entry *dstroute;
        /* Lookup the route to the destination */
        dstroute = route_lookup(rreq->dstip);

        if(dstroute != NULL && dstroute->complete) {
            /* dstroute is a complete route to the destination */
            if(send_rrep(rreq, srcroute, dstroute->numhops) < 0) {
                return 0;
            }
            /* Set the flags for already sent */
            rreq->flags |= ODR_RREP_SENT;
            /* Continue to broadcast RREQ to everyone except source if_index */
        }
        /* broadcast RREQ to everyone except source if_index */
        return broadcast_rreq(rreq, llsrc->sll_ifindex);
    }
    return 1;
}

/*
 * @param rrep    Pointer to a valid RREP type odr_msg
 * @param llsrc   Link layer source address of the RREP message
 */
int process_rrep(struct odr_msg *rrep, struct sockaddr_ll *llsrc) {
    return 0;
}

/*
 * @param data    Pointer to a valid DATA type odr_msg
 * @param llsrc   Link layer source address of the DATA message
 */
int process_data(struct odr_msg *data, struct sockaddr_ll *llsrc) {
    return 0;
}

/*
 * @param rreq         The RREQ message to construct and send an RREP
 * @param route        The route back to the source of the RREQ
 * @param hops_to_dst  The number of hops to the destination of the RREQ
 *                     This should be 0 if we are the destnation.
 * @return 1 sent. 0 not sent. -1 error.
 */
int send_rrep(struct odr_msg *rreq, struct route_entry *route,
        int32_t hops_to_dst) {
    struct odr_msg rrep;

    /* route to the source of the RREQ. This should always exist */
    if(route == NULL || !route->complete) {
        error("Route to source must be complete!\n");
        return -1;
    }

    if(rreq->flags & ODR_RREP_SENT) {
        debug("An intermediate node already sent an RREP for this RREQ.\n");
        return 0;
    }

    /* constuct RREP */
    memset(&rrep, 0, sizeof(rrep));
    rrep.type = ODR_RREP;
    rrep.flags = (rreq->flags & ODR_FORCE_RREQ)? ODR_FORCE_RREQ : 0;
    rrep.srcip = rreq->dstip;
    rrep.dstip = rreq->srcip;
    rrep.numhops = hops_to_dst + 1;

    /* send the RREP back to the RREQ source */
    if(!send_frame(&rrep, route->nxtmac, route->outmac, route->if_index)) {
        /* send failed */
        return -1;
    }
    return 1;
}

/*
 * Broadcast rreq out on every interface index except src_ifindex. If
 * src_ifindex is -1 then it will be broadcasted of all interfaces.
 *
 * @param rreq        The RREQ to broadcast
 * @param src_ifindex The interface index that the RREQ was received on OR -1 if
 *                    this ODR node needs to send it on every interface.
 * @return 1 if succeeded 0 if failed
 */
int broadcast_rreq(struct odr_msg *rreq, int src_ifindex) {
    struct hwa_info *cur;
    unsigned char broadmac[ETH_ALEN];

    /* Broadcast address is all 0xFF */
    memset(broadmac, 0xFF, sizeof(broadmac));
    if(rreq == NULL) {
        error("RREQ cannot be NULL\n");
        return 0;
    }

    for(cur = hwahead; cur != NULL; cur = cur->hwa_next) {
        if(cur->if_index != src_ifindex) {
            /* send ethernet frame to cur->ifindex */
            if(!send_frame(rreq, broadmac, cur->if_haddr, cur->if_index)) {
                /* send failed */
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Construct and send an ethernet frame to the dst_hwaddr MAC from src_hwaddr
 * MAC going out of interface index ifi_index.
 *
 * @param payload    Pointer to the ODR message to send, in HOST byte order
 * @param dst_hwaddr The next hop MAC address
 * @param src_hwaddr The outgoing MAC address
 * @param ifi_index  The outgoing interface index in HOST byte order
 * @return 1 if succeeded 0 if failed
 */
int send_frame(struct odr_msg *payload, unsigned char *dst_hwaddr,
        unsigned char *src_hwaddr, int ifi_index) {
    char frame[ETH_FRAME_LEN]; /* MAX ethernet frame length 1514 */
    struct ethhdr *eh = (struct ethhdr *)frame;
    struct sockaddr_ll dest;
    int nsent, size;

    size = ODR_MSG_SIZE(payload);

    if(size > ETH_DATA_LEN) {
        error("Frame data too large: %d\n", size);
        return 0;
    }
    /* Initialize ethernet frame */
    memcpy(eh->h_dest, dst_hwaddr, ETH_ALEN);
    memcpy(eh->h_source, src_hwaddr, ETH_ALEN);
    eh->h_proto = htons(ETH_P_ODR);
    /* Convert payload into NETWORK byte order */
    hton_msg(payload);
    /* Copy frame data into buffer */
    memcpy(frame + sizeof(struct ethhdr), payload, size);
    /* Convert payload back into HOST byte order */
    ntoh_msg(payload);
    /* Initialize sockaddr_ll */
    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_ifindex = ifi_index;
    memcpy(dest.sll_addr, dst_hwaddr, ETH_ALEN);
    dest.sll_halen = ETH_ALEN;

    debug("Sending frame TODO: print frame\n");

    if((nsent = sendto(packsock, frame, size+sizeof(struct ethhdr), 0,
            (struct sockaddr *)&dest, sizeof(dest))) < 0) {
        error("packet sendto: %s\n", strerror(errno));
        return 0;
    }
    return 1;
}

/*
 * Receives a ethernet frame and stores the ODR message into recvmsg. Converts
 * the message into host order as well.
 * @Return is the same as recvfrom(2)
 */
ssize_t recv_frame(struct ethhdr *eh, struct odr_msg *recvmsg,
        struct sockaddr_ll *src) {
    char frame[ETH_FRAME_LEN]; /* MAX ethernet frame length 1514 */
    socklen_t srclen;
    ssize_t nread;

    if((nread = recvfrom(unixsock, frame, sizeof(frame), 0,
            (struct sockaddr *)src, &srclen)) < 0) {
        error("packet socket recv failed: %s\n", strerror(errno));
    } else {
        /* Copy ethernet frame header into eh */
        memcpy(eh, frame, ETH_HLEN);
        /* Copy the frame_data into the odr_msg */
        memcpy(recvmsg, frame + ETH_HLEN, nread - ETH_HLEN);
        /* Convert message from Network to Host order */
        ntoh_msg(recvmsg);
    }
    return nread;
}

/*
 * Returns the current timestamp in microseconds. Can not fail.
 */
uint64_t usec_ts(void) {
    struct timespec tp;
    uint64_t ts;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    ts = ((uint64_t)tp.tv_sec) * 1000000ULL;
    ts += ((uint64_t)tp.tv_nsec) / 1000ULL;
    return ts;
}

/*
 * Returns true if the MAC addresses are equal
 */
int samemac(unsigned char *mac1, unsigned char *mac2) {
    return (memcmp(mac1, mac2, ETH_ALEN) == 0);
}

/*
 * Converts odr_msg from Host to Network byte order
 */
void hton_msg(struct odr_msg *msg) {
    msg->srcport     = htonl(msg->srcport);
    msg->dstport     = htonl(msg->dstport);
    msg->numhops     = htonl(msg->numhops);
    msg->broadcastid = htonl(msg->broadcastid);
    msg->dlen        = htonl(msg->dlen);
}

/*
 * Converts odr_msg from Network to Host byte order
 */
void ntoh_msg(struct odr_msg *msg) {
    msg->srcport     = ntohl(msg->srcport);
    msg->dstport     = ntohl(msg->dstport);
    msg->numhops     = ntohl(msg->numhops);
    msg->broadcastid = ntohl(msg->broadcastid);
    msg->dlen        = ntohl(msg->dlen);
}

/*********************** BEGIN routing table functions ************************/

/*
 * Adds a complete route entry to the table, or updates an incomplete entry.
 * If it updates an incomplete entry then it sends out the list of queued
 * messages for the route.
 *
 * @param nxtmac   The MAC address of the next hop on the way to destination ip
 * @param dstip    The canonical address of the route's eventual destination
 * @param ifiindex The outgoing interface index in host byte order
 * @param numhops  The number of hops to the destination in host byte order
 *
 * @return 1 if added/updated a route, 0 if did not update, -1 if error
 */
int route_add_complete(unsigned char *nxtmac, struct in_addr dstip, int ifindex,
        int numhops) {
    struct route_entry *old;
    struct hwa_info *hwa_out;

    /* get the info for this interface index */
    if((hwa_out = hwa_searchbyindex(hwahead, ifindex)) == NULL) {
        error("Outgoing interface index %d not found!\n", ifindex);
        return -1;
    }

    /* Look for an existing route */
    old = route_lookup(dstip);
    if(old == NULL) {
        /* No route exists to the destination yet, so create one */
        struct route_entry *new;
        if((new = malloc(sizeof(struct route_entry))) == NULL) {
            error("malloc failed: %s\n", strerror(errno));
            return -1;
        }
        route_entry_update(new, nxtmac, hwa_out->if_haddr, ifindex, numhops);
        /* add this new route entry to the route table */
        new->head = NULL;
        new->next = routehead;
        routehead = new;
    } else if(old->complete) {
        /* check if this new route is more efficient, or same but different */
        if(numhops < old->numhops || (numhops == old->numhops &&
                (!samemac(nxtmac, old->nxtmac) || ifindex != old->if_index))) {
            /* We need to update this entry */
            route_entry_update(old, nxtmac, hwa_out->if_haddr, ifindex,numhops);
        } else {
            /* Return 0 because we did not update this route */
            return 0;
        }
    } else {
        struct msg_node *nxt;
        /* Incomplete route to dstip exists, update it AND send messages */
        route_entry_update(old, nxtmac, hwa_out->if_haddr, ifindex,numhops);
        /* Send all the queued messages */
        for(;old->head != NULL; old->head = nxt) {
            nxt = old->head->next;
            debug("Completed route, sending queued messages\n");
            if(!send_frame(old->head->msg, old->nxtmac, old->outmac,
                    old->if_index)) {
                /* send failed */
                return -1;
            }
            /* free the memory */
            free(old->head->msg);
            free(old->head);
        }
    }
    return 1;
}

void route_entry_update(struct route_entry *r, unsigned char *nxtmac,
        unsigned char *outmac, int if_index, int numhops) {
    /* We need to update this entry */
    r->complete = 1;
    r->ts = usec_ts();
    r->numhops = numhops;
    r->if_index = if_index;
    memcpy(r->nxtmac, nxtmac, ETH_ALEN);
    memcpy(r->outmac, outmac, ETH_ALEN);
}

/*
 * Remove the route to dest
 */
void route_remove(struct in_addr dest) {
    struct route_entry *tmp, *prev, *next;

    prev = NULL;
    for(tmp = routehead; tmp != NULL; tmp = tmp->next) {
        next = tmp->next;
        if(tmp->dstip.s_addr == dest.s_addr) {
            /* Delete the node if it is complete and has no queued messages */
            if(tmp->complete) {
                if(tmp->head == NULL) {
                    debug("Removing route to dest node %s\n", inet_ntoa(dest));
                    free(tmp);
                    if(prev == NULL) {
                        routehead = next;
                    } else {
                        prev->next = next;
                    }
                } else {
                    error("Complete route entry has queued messages!\n");
                }
            }
            return;
        }
        prev = tmp;
    }
}

/*
 * Search for a route to dest in the routing table and return pointer.
 */
struct route_entry *route_lookup(struct in_addr dest) {
    struct route_entry *tmp;

    for(tmp = routehead; tmp != NULL; tmp = tmp->next) {
        if(tmp->dstip.s_addr == dest.s_addr) {
            /* found the node! */
            return tmp;
        }
    }
    return NULL;
}

/*
 * Removes all stale entries in the routing table.
 */
void route_cleanup(void) {
    uint64_t ts;
    struct route_entry *cur, *next, *prev;

    /* get the current usec timestamp */
    ts = usec_ts();

    prev = NULL;
    cur = routehead;
    while(cur != NULL) {
        next = cur->next;
        /* Cleanup */
        if(ts - cur->ts > route_ttl) {
            /* remove stale node */
            free(cur);
            if(prev == NULL) {
                /* we are removing the head */
                routehead = next;
            } else {
                prev->next = next;
            }
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void route_free(void) {
    struct route_entry *tmp;

    while(routehead != NULL) {
        tmp = routehead->next;
        free(routehead);
        routehead = tmp;
    }
}

/************************ END routing table functions *************************/

/************************* BEGIN Port Table functions *************************/

/*
 * Add the well known port, and sockaddr_un of the local instance of the server.
 */
struct port_node *init_port_table(void) {
    struct port_node *serv;

    if((serv = malloc(sizeof(struct port_node))) == NULL) {
        error("malloc failed: %s\n", strerror(errno));
        return NULL;
    }

    /* Copy server UNIX domain socket path */
    serv->unaddr.sun_family = AF_UNIX;
    strncpy(serv->unaddr.sun_path, SERVER_PATH,
            sizeof(serv->unaddr.sun_path) - 1);
    serv->port = SERVER_PORT;
    serv->next = NULL;
    serv->permanent = 1;
    serv->ts = usec_ts();
    return serv;
}

/*
 * Search for a port_node by port number
 */
struct port_node *port_searchbyport(int port) {
    struct port_node *tmp;

    for(tmp = porthead; tmp != NULL; tmp = tmp->next) {
        if(tmp->port == port) {
            /* found the node! */
            return tmp;
        }
    }
    return NULL;
}

/*
 * Search for a port_node by UNIX socket address
 */
struct port_node *port_searchbyaddr(struct sockaddr_un *unaddr) {
    struct port_node *tmp;

    for(tmp = porthead; tmp != NULL; tmp = tmp->next) {
        if(strncmp(tmp->unaddr.sun_path, unaddr->sun_path,
                sizeof(unaddr->sun_path)) == 0) {
            /* found the node! */
            return tmp;
        }
    }
    return NULL;
}

/*
 * Add a UNIX domain socket address to the port table. Port table is stored in
 * increasing port order.
 *
 * @return  The newly allocated port or -1 if malloc failed
 */
int port_add(struct sockaddr_un *addr) {
    struct port_node *newnode, *cur, *prev;

    if((newnode = malloc(sizeof(struct port_node))) == NULL) {
        /* malloc failed */
        error("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    /* Init the new node */
    memcpy(&newnode->unaddr, addr, sizeof(struct sockaddr_un));
    newnode->port = 0;
    newnode->next = NULL;

    /* Search for the lowest unused port and insert */
    for(prev = NULL, cur = porthead; cur != NULL; prev = cur, cur = cur->next,
            newnode->port++) {
        if(newnode->port < cur->port) {
            /* add new node here */
            newnode->next = cur;
            if(prev == NULL) {
                /* push onto head of the list */
                porthead = newnode;
            } else {
                prev->next = newnode;
            }
            return newnode->port;
        }
    }
    /* Push this node onto the port list */
    newnode->next = porthead;
    porthead = newnode;
    return newnode->port;
}

/*
 * Free all the nodes in the port allocation list
 */
void port_free(void) {
    struct port_node *tmp;

    while(porthead != NULL) {
        tmp = porthead->next;
        free(porthead);
        porthead = tmp;
    }
}

/************************** END Port Table functions **************************/

/********************* BEGIN Previous RREQ list functions *********************/

struct bid_node *bid_lookup(struct in_addr src) {
    struct bid_node *tmp;

    for(tmp = bidhead; tmp != NULL; tmp = tmp->next) {
        if(tmp->srcip.s_addr == src.s_addr) {
            /* found the node! */
            return tmp;
        }
    }
    return NULL;
}

/*
 * Add a Broadcast ID entry to mark that we have seen this bid from source ip.
 */
int bid_add(struct odr_msg *rreq) {
    struct bid_node *newbid;

    if((newbid = malloc(sizeof(struct bid_node))) == NULL) {
        /* malloc failed */
        error("malloc failed: %s\n", strerror(errno));
        return 0;
    }
    /* Init the new node */
    newbid->srcip.s_addr = rreq->srcip.s_addr;
    newbid->broadcastid = rreq->broadcastid;
    newbid->numhops = rreq->numhops;
    /* Push this node onto the bid list */
    newbid->next = bidhead;
    bidhead = newbid;
    return 1;
}

/*
 * Free all the nodes in the bid_node list
 */
void bid_free(void) {
    struct bid_node *tmp;

    while(bidhead != NULL) {
        tmp = bidhead->next;
        free(bidhead);
        bidhead = tmp;
    }
}

/*
 * Returns true if this rreq is a duplicate (Already been processed). and the
 * numhops of rreq is less efficeint than the previous rreq.
 *
 */
int ignore_rreq(struct odr_msg *rreq) {
    struct bid_node *prev;

    /* Previous RREQ will have the same source and an equal or lower bid */
    prev = bid_lookup(rreq->srcip);
    if(prev == NULL) {
        return 0;
    } else if(prev->broadcastid > rreq->broadcastid) {
        return 1;
    } else if(prev->broadcastid == rreq->broadcastid) {
        /* ignore duplicate RREQs that are  */
        return prev->numhops <= rreq->numhops;
    } else {
        return 0;
    }
}

/********************* END Previous RREQ list functions ***********************/

static void cleanup(int signum) {
    /* remove the UNIX socket file */
    unlink(ODR_PATH);
    /* 128+n Fatal error signal "n" is the standard Linux exit code */
    exit(128 + signum);
}

static void set_sig_cleanup(void) {
    struct sigaction sigac_int;
    /* Zero out memory */
    memset(&sigac_int, 0, sizeof(sigac_int));
    /* Set values */
    sigac_int.sa_handler = &cleanup;
    /* Set the sigaction */
    if(sigaction(SIGINT, &sigac_int, NULL) < 0) {
        error("sigaction failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
