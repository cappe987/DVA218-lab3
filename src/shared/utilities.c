#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/time.h> 
#include <netinet/in.h> 
#include "utilities.h"
#include "constants.h"
#include "base_packet.h"
#include "induce_errors.h"
#include "crc32.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>


void reset_variables(int *timeout, int sockfd, struct timeval *tv){
    *timeout = 0;
    tv->tv_sec = TIMEOUT;
    struct timeval temp = *tv;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    perror("Error");
  }
}

void reset_timeout(int *timeout, int sockfd, struct timeval *tv){
    *timeout = 0;
    tv->tv_sec = TIMEOUT;
    struct timeval temp = *tv;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    perror("Error");
  }
}

void increment_timeout(int *timeout, int sockfd, struct timeval *tv){
  (*timeout)++;
  tv->tv_sec = tv->tv_sec * 2;
  struct timeval temp = *tv;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    perror("Error");
  }
}

ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr){
  socklen_t len = sizeof(sockaddr);
  base_packet packet;
  packet.data[0] = '\0';
  packet.seq = seq;
  packet.flags = flag;
  printf("Sent ");
  if(flag % 2  == 1){printf("ACK (SEQ %d)", seq); flag = flag - 1;}
  if(flag % 4  == 2){printf("SYN "); flag = flag - 2;}
  if(flag % 8  == 4){printf("FIN "); flag = flag - 4;}
  if(flag % 16 == 8){printf("NACK ");}

  printf("\n");

  crc_packet full_packet = create_crc((char*)&packet);
  return send_with_error(sockfd, (const char *)&full_packet, sizeof(crc_packet),  
      MSG_CONFIRM, (const struct sockaddr *)&sockaddr, len); 

}

int error_check(int read, crc_packet packet){
  if(read == 0){ // Socket has shut down, not sure if needed
    printf(">>> Socket closed for some reason\n");
    exit(1);
    // return false;
  }
  else if(read < 0){
    // printf(">>> Error on recvfrom |%s|\n", strerror(errno));
  }
  else { // Successful read
    return valid_crc(packet);
  }
  return false;
}

