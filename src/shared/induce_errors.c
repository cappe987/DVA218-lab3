#include "induce_errors.h"

ssize_t send_with_error(int sockfd, const void* buf, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){

  if(ERRORS_ENABLED){ // ERRORS_ENABLED can be found in induce_errors.h
    // Do printf whenever an error occurs.

  }
  else{
    return sendto(sockfd, buf, size, flags, addr, addr_len); 
  }

}