#ifndef SHARED_FUNCTIONS_H
#define SHARED_FUNCTIONS_H
#include <sys/time.h>


void reset_variables(int *timeout, int *response, struct timeval *tv);

void increment_timeout(int *timeout, int *response, int sockfd, struct timeval *tv);

#endif