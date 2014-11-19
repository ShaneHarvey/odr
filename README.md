odr
===

Network Programming Assignment 3

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
                /* Construct odr_msg and incomplete route entry */
                add an incomplete entry to the routing table with route.dstip =
                to the destination address and route.msg_head = odr_msg.
                /* Next, Broadcast an RREQ out on each interface */

    recvfrom on packet socket
        /* do stuff */
