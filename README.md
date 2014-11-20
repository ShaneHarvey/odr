odr
===

Network Programming Assignment 3

```
ODR
interface list
    get_hw_addrs()
    remove eth0 and lo interfaces from the list

routing table
    A list of (complete or incomplete) route entries.
    A route entry can be incomplete (like ARP cache entries).
    Each (incomplete) route has a queue of messages to send out.
    When a route entry is completed send all messages in the queue.

port allocation table
    A list of port entries. Contains a port, a sockaddr_un, and timestamp.
    The sockaddr_un is the UNIX domain socket source of an API message.
    Initially contains the local time server's well known path and port
    as a permanent entry.

recvfrom on UNIX domain socket
    ? Clean up stale port entries? 1 minute old ?
    if port_lookup(sockaddr_un) fails to find a port
        add a port entry with a unique port for sockaddr_un.

    if Destination address is local (localhost, or current node)
        if destination port number in port table
            sendto destination sockaddr_un, api_msg
        else
            print WARN: unknown port %d, dropping message
    else Destination address is non-local
        remove_stale_routes()
        if api_msg.flag <----- FORCE_RREQ
            remove_route(destination address)

        if incomplete_route(destination address)
            append odr_msg to the incomplete route entry's message queue
        else if complete_route(destination address)
            construct type 2 application payload Ethernet frame
            sendto outgoing interface, MAC from the existing route
        else
            /* Construct odr_msg and incomplete route entry 
             * add an incomplete entry to the routing table with route.dstip =
             * to the destination address and route.msg_head = odr_msg.
             */
            add_incomplete_route()
            /* Next, Broadcast an RREQ out on each interface */
            broadcast_rreq(all) 

recvfrom on packet socket
    /* Update route table */
    remove_stale_routes()
    if FORCE_RREQ
        remove_route(dest ip)
    add_route to source MAC <-------Update if shorter numhops OR same hops
                                    but diff ifindex or MAC
    /* proces ODR message */
    if msg.type == RREQ
        if msg.dstip == thisnode
            send_rrep()
        else if complete_route(msg.dstip)
            was_sent = send_rrep()
            if was_sent && previously_unknown source
                set flags to ODR_RREP_SENT
                broadcast RREQ to each interface except source
        else if bid_lookup(srcip, broadcastid) == NULL
            broadcast RREQ to each interface except source
        else
            /* duplicate RREQ ignored */
    else if RREP
        /* TDOD: Finish */
        if msg.dstip == thisnode
            /* do nothing cause we already added route? */
        else if incomplete_route(msg.dstip)
            /* search odr_msg queue for same RREP (src/dst IP the same)
             * if found
             *     if found.numhops > msg.numhops
             *          Update the already queued msg numhops :)
             *          found.numhops = msg.numhops
             *     else
             *         drop the duplicate RREP msg there is a better RREP
             * else
             *     append msg to this incomplete route's queue
             */
        else if complete_route(msg.dstip)
            /* Do not forward suboptimal RREPs */
        else
            /* No route exists, send RREQ for msg.dstip */
            add_incomplete_route()
            broadcast_rreq(all except src_ifindex) 
    else if DATA
        if msg.dstip == thisnode
            /* send message to destination port (sockaddr_un) */
        else if incomplete_route(msg.dstip)
            append msg to this incomplete route's queue
        else if complete_route(msg.dstip)
            /* Forward msg along the route */
        else
            /* No route exists, send RREQ for msg.dstip */
            add_incomplete_route()
            broadcast_rreq(all except src_ifindex) 
    else
        warn invalid message type
```

```c
function send_rrep() {
    was_sent = false
    bid_node = bid_lookup(srcip, broadcastid)
    if(bid_node == NULL) {
        send RREP message
        add_bid_node(srcip, broadcastid, new_numhops)
        was_sent = true
    } else if(new_numhops < bid_node->numhops) {
        send RREP message
        add_bid_node(srcip, broadcastid, new_numhops)
        was_sent = true
    }
    return was_sent
}
```
