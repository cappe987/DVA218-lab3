#ifndef RECEIVER_TEARDOWN_H
#define RECEIVER_TEARDOWN_H
#include <sys/socket.h> 


void connection_teardown(int sockfd, struct sockaddr_in cliaddr, socklen_t len, int seqNr);

#endif