#ifndef CLIENT_H
#define CLIENT_H
/* libc headers */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
/* System headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Program headers */
#include "debug.h"
#include "api.h"
#include "common.h"
/* Preprocessor defines */
#define BUFFER_SIZE 1024

/* Program prototypes */

/**
 * Starts running the client program.
 * @param sock_fd File descriptor bound to the temp file used for IPC.
 * @return Returns the success of failure conditions of the running process.
 */
int run(int sock_fd);

/**
 * ctrl-c signal handler.
 * @param signal
 */
void intHandler(int signal);
#endif
