#include "client.h"

static bool running = false;

int main(int argc, char *argv[]) {
    int success = EXIT_SUCCESS;
    char temp_name[] = "hw3_tempXXXXXX";
    int sock_fd;
    struct sockaddr_un local;
    /* set ctrl-c to remove temp files */
    signal(SIGINT, intHandler);
    /* Attempt to open the temp file. */
    if(mkstemp(temp_name) < 0) {
        error("Failed to create the tempfile %s - %s\n", temp_name, strerror(errno));
        success = EXIT_FAILURE;
        goto EXIT;
    }
    /* Unlink the file so we can bind the socket to the file... */
    if(unlink(temp_name) == 0) {
        debug("Sucessfully removed %s\n", temp_name);
    } else {
        error("Failed to remove the tempfile %s - %s\n", temp_name, strerror(errno));
        success = EXIT_FAILURE;
        goto EXIT;
    }
    /* Attempt to get a socket */
    if((sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        error("Failed to get a socket - %s\n", strerror(errno));
        success = EXIT_FAILURE;
        goto REMOVE_TMP;
    }
    /* Attempt to bind to the socket */
    memset(&local, 0, sizeof(struct sockaddr_un));
    /* Copy the path to the temp file to the local.sun_path */
    local.sun_family = AF_LOCAL;
    strcpy(local.sun_path, temp_name);
    /* Attempt to bind socket */
    if(bind(sock_fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
        error("Failed to bind to the sock_fd: %d - %s\n", sock_fd, strerror(errno));
        success = EXIT_FAILURE;
        goto CLOSE_SOCK_FD;
    }
    /* If we got this far we can attempt to enter the run loop */
    success = run(sock_fd);
CLOSE_SOCK_FD:
    /* Attempt to close the open socket */
    if(sock_fd >= 0) {
        if(close(sock_fd) == 0) {
            debug("Successfully closed sock_fd: %d\n", sock_fd);
        } else {
            warn("Failed to close sock_fd: %d\n", sock_fd);
        }
    }
REMOVE_TMP:
    /* Attempt to remove the temp file */
    if(unlink(temp_name) == 0) {
        debug("Sucessfully removed %s\n", temp_name);
    } else {
        warn("Failed to remove %s - %s\n", temp_name, strerror(errno));
    }
EXIT:
    return success;
}

static bool isValidVM(const char *vm) {
    bool valid = false;
    if(vm != NULL) {
        int len = strlen(vm);
        if(len > 2 && len < 5 && vm[0] == 'v' && vm[1] == 'm') {
            char *error = NULL;
            // Increment the pointer by 2
            vm += 2;
            // Parse the number
            int vm_num = strtol(vm, &error, 10);
            if(vm_num > 0 && vm_num < 11 && *error == '\0') {
                valid = true;
            } else {
                warn("Invalid VM '%s' provided.\n", vm);
            }
        }
    }
    return valid;
}

static char* getIpaddress(const char *hostname) {
    char *ipaddress = NULL;
    if(hostname != NULL) {
        struct hostent *he;
        struct in_addr **addr_list;
        he = gethostbyname(hostname);
        if(he != NULL) {
            addr_list = (struct in_addr**)he->h_addr_list;
            // Just get the first address
            if(addr_list[0] != NULL) {
                char *cp_ip = inet_ntoa(*addr_list[0]);
                ipaddress = malloc(strlen(cp_ip) + sizeof(char));
                strcpy(ipaddress, cp_ip);
            }
        } else {
            error("Unable to get host by name.\n");
            herror("");
        }
    }
    return ipaddress;
}

int run(int sock_fd) {
    int success = EXIT_SUCCESS;
    running = true;
    char buffer[BUFFER_SIZE];
    char vm[BUFFER_SIZE];
    while(running) {
        printf("Select a vm: vm1, . . . . . , vm10\n> ");
        if(fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            int len = strlen(buffer);
            // Remove the newline from the string
            if(buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            // See if we have a VM
            if(sscanf(buffer, "%s", vm) == 1) {
                // Check to see if we have a valid VM
                if(isValidVM(vm)) {
                    // Send msg
                    char *msg = "w";
                    char *canonicalIP = getIpaddress(vm);
                    if(canonicalIP != NULL) {
                        char this_vm[BUFFER_SIZE];
                        if(gethostname(this_vm, BUFFER_SIZE) == 0) {
                            info("client at node %s sending request to server at %s\n", this_vm, vm);
                            int resendAttempts = 0;
                            if(msg_send(sock_fd, msg, sizeof(msg), canonicalIP, SERVER_PORT, 0) == strlen(msg)) {
                                do {
                                    // select vars
                                    fd_set rset;
                                    struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
                                    // Now wait to receive the message
                                    FD_ZERO(&rset);
                                    FD_SET(sock_fd, &rset);
                                    select(sock_fd + 1, &rset, NULL, NULL, &tv);
                                    if(FD_ISSET(sock_fd, &rset)) {
                                        char received_ip[BUFFER_SIZE];
                                        char received_msg[BUFFER_SIZE];
                                        int received_port = 0, bytes = 0;
                                        if((bytes = msg_recv(sock_fd, received_msg, sizeof(received_msg),
                                                received_ip, sizeof(received_ip), &received_port)) > 0) {
                                            info("client at node %s : received from %s <%s>", this_vm, vm, received_msg);
                                            // Exit the dowhile loop
                                            break;
                                        } else {
                                            warn("Failed to receive.\n");
                                        }
                                    } else if(resendAttempts < 1) {
                                        warn("client at node %s : timeout on response from %s\n", this_vm, vm);
                                        // Send again with routing discovery
                                        if(msg_send(sock_fd, msg, sizeof(msg), canonicalIP, SERVER_PORT, 1) != strlen(msg)) {
                                            error("msg_send with discovery failed: %s\n", strerror(errno));
                                            running = false;
                                            // Get our of this loop
                                            break;
                                        }
                                    }
                                } while(resendAttempts++ < 1);
                            } else {
                                error("msg_send failed: %s\n", strerror(errno));
                                running = false;
                            }
                        } else {
                            error("Failed to get the hostname of this node\n");
                        }
                        // Free the dynamiclly allocated memory
                        free(canonicalIP);
                    } else {
                        error("Unable to obtain canonical ipaddress for %s.\n", vm);
                        running = false;
                    }
                } else {
                    // Invalid VM Name provided
                    goto INVALID_VM;
                }
            } else {
                // Invalid VM name provided
                goto INVALID_VM;
            }
        } else {
            // EOF chaarcter was sent.
            printf("\n");
            running = false;
INVALID_VM:
            if(running) {
                warn("Invalid vm `%s` selected.\n", buffer);
            }
        }
    }
    return success;
}

void intHandler(int signal) {
    running = false;
    fclose(stdin);
}
