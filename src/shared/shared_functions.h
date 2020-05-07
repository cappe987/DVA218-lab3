#ifndef SHARED_FUNCTIONS_H
#define SHARED_FUNCTIONS_H
#include <sys/time.h>
#include <sys/socket.h> 


void reset_variables(int *timeout, int *response, struct timeval *tv);

void increment_timeout(int *timeout, int *response, int sockfd, struct timeval *tv);

ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr);

#endif