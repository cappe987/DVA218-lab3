#include "shared_functions.h"
#include "constants.h"
#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/time.h> 

void reset_variables(int *timeout, int *response, struct timeval *tv){
    *timeout = 0;
    *response = -1;
    tv->tv_sec = TIMEOUT;
}

void increment_timeout(int *timeout, int *response, int sockfd, struct timeval *tv){
  (*timeout)++;
  tv->tv_sec = tv->tv_sec * 2;
  *response = -1;
  struct timeval temp = *tv;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    perror("Error");
  }
}