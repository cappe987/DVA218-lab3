#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/time.h> 
#include <netinet/in.h> 
#include "utilities.h"
#include "constants.h"
#include "base_packet.h"
#include "induce_errors.h"
#include "crc32.h"


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

ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr){
  socklen_t len = 0;
  base_packet packet;
  packet.data[0] = '\0';
  packet.seq = seq;
  packet.flags = flag;
  printf("Sent ");
  if(flag == 1){printf("ACK"); flag = flag - 1;}
  if(flag == 2){printf("SYN"); flag = flag - 2;}
  if(flag == 4){printf("FIN"); flag = flag - 4;}
  if(flag == 8){printf("NACK");}

  printf("\n");

  crc_packet full_packet = create_crc((char*)&packet);
  return send_with_error(sockfd, (const char *)&full_packet, sizeof(crc_packet),  
      MSG_CONFIRM, (const struct sockaddr *) &sockaddr, len); 

}