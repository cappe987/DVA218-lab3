#ifndef UTILITIES_H
#define UTILITIES_H
#include <sys/time.h>
#include <sys/socket.h> 
#include "crc32.h"


void reset_variables(int *timeout, int sockfd, struct timeval *tv);

void reset_timeout(int *timeout, int sockfd, struct timeval *tv);

void increment_timeout(int *timeout, int sockfd, struct timeval *tv);

ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr);

int error_check(int read, crc_packet packet);

#endif