#ifndef ODR_H
#define ODR_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include "common.h"
#include "get_hw_addrs.h"
#include "debug.h"

#define ODR_PROTOCOL 62239

int run_odr(int unixsock, int rawsock);

#endif
