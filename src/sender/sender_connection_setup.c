#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/time.h> 
#include <time.h> 
#include "../shared/base_packet.h"
#include "../shared/induce_errors.h"
#include "../shared/constants.h"  
#include "../shared/utilities.h"
#include "sender_connection_setup.h"

int connection_setup(int sockfd, struct sockaddr_in servaddr){
  int response = -1, seq = -1, nr_of_timeouts = 0;
  char* message_to_rec;
  char buffer[DATA_SIZE]; 
  base_packet packet;
  base_packet packet_received;
  socklen_t len;
  //socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
    {
      perror("Error");
    }

  srand(time(0));
  seq = rand() % 100;
  // printf("%d", seq);
  // Setting sequence number and SYN
  packet.seq = seq;
  packet.flags = 2;
  
  message_to_rec = (char*)&packet;     
  send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  printf("SYN sent.\n");

  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
    packet_received = *(base_packet*) buffer;
    printf("Server : %d\n", packet_received.flags); 

    if(packet_received.flags != 3){
      message_to_rec = (char*)&packet;       
      send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
      
      printf("Received NACK or timeout, SYN resent.\n");
      increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
      if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
    }
  }

  reset_variables(&nr_of_timeouts, &response, &tv);

  seq++;
  packet.seq = seq;
  packet.flags = 1;
  message_to_rec = (char*)&packet; 
  send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  printf("SYN + ACK received, sending ACK.\nConnection Established\n"); 

  return seq;
}