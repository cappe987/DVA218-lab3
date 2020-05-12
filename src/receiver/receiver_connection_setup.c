#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/time.h> 
#include "../shared/base_packet.h"
#include "../shared/induce_errors.h"
#include "../shared/constants.h"  
#include "../shared/utilities.h"
#include "receiver_connection_setup.h"
#include "../shared/crc32.h"

int connection_setup(int sockfd, struct sockaddr_in cliaddr){
  char buffer[DATA_SIZE]; 
  base_packet packet;
  base_packet packet_received;
  crc_packet full_packet;
  socklen_t len;
  //socket timeout
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
    perror("Error");
  }
  len = sizeof(cliaddr);  //len is value/result
  packet.seq = 1;
  packet.flags = 3;
  int response = -1, nr_of_timeouts = 0, sender_seq = -1;

  
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

    printf("Sender: %d\n", packet_received.flags);

    if( ! error_check(response, full_packet)){
      // Send NACK
      printf(">>> Failed CRC check\n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      continue;
    }

    if(packet_received.flags != 2){
        printf("Received faulty SYN / NACK or timeout\n");
        send_without_data(packet.seq, 8, sockfd, cliaddr); 

        increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
    }
    sender_seq = packet_received.seq;
  }
  
  reset_variables(&nr_of_timeouts, &response, &tv);

  sender_seq++;
  packet.seq = sender_seq; 
  send_without_data(packet.seq, 3, sockfd, cliaddr);

  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

    printf("Sender: %d\n", packet_received.flags);

    if( ! error_check(response, full_packet)){
      // Send NACK
      printf(">>> Failed CRC check\n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      continue;
    }

    if(packet_received.flags != 1){
        printf("Received NACK or timeout\n");
        send_without_data(packet.seq, 3, sockfd, cliaddr);
        
        increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
    } 
    sender_seq = packet_received.seq;
  }

  reset_variables(&nr_of_timeouts, &response, &tv);

  tv.tv_sec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
    perror("Error");
  }

  printf("Connection established\n");
  return sender_seq;
}