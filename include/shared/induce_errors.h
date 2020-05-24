#ifndef INDUCE_ERRORS_H
#define INDUCE_ERRORS_H
#include <sys/socket.h> 


ssize_t send_with_error(int sockfd, const void* buf, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len);


#endif