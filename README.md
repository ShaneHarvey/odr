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
    Stale route are cleaned up

port allocation table
    Port allocations are discarded after 20 seconds of inactivity.
    A list of port entries. Contains a port, a sockaddr_un, and timestamp.
    The sockaddr_un is the UNIX domain socket source of an API message.
    Initially contains the local time server's well known path and port
    as a permanent entry.
