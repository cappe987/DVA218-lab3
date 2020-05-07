#ifndef RECEIVER_CONNECTION_SETUP_H
#define RECEIVER_CONNECTION_SETUP_H
#include <sys/socket.h> 

int connection_setup(int sockfd, struct sockaddr_in cliaddr);

#endif