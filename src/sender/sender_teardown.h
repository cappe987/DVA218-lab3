#ifndef SENDER_TEARDOWN_H
#define SENDER_TEARDOWN_H

int connection_teardown(int sockfd, struct sockaddr_in servaddr, int sequence);

#endif