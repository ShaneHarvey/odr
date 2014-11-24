odr
===

Network Programming Assignment 3

Shane Harvey 108272239
Paul Campbell 108481554

###ODR

To build
```bash
$ make
```
To run ODR
```bash
$ sudo ./ODR_cse533-14 120
```
To run server
```bash
$ ./server_cse533-14
```
To run client
```bash
$ ./client_cse533-14
```

It evens works with a staleness argument of 0 seconds :)

interface list
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
