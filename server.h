#ifndef SERVER_H
#define SERVER_H
/* libc headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
/* System headers */
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
/* Program headers */
#include "common.h"
#include "debug.h"
#include "api.h"
/* Program prototypes */

void run_time_server(int unix_socket);
#endif
