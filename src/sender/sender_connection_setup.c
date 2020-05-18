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
#include "../shared/crc32.h"

int connection_setup(int sockfd, struct sockaddr_in servaddr){
  int response = -1, seq = -1, nr_of_timeouts = 0;
  char buffer[sizeof(crc_packet)]; 
  base_packet packet;
  base_packet packet_received;
  crc_packet full_packet;
  socklen_t len;
  //socket timeout
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  //Sets timeout
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
    time_stamp();
    perror("Error\n");
  }

  seq = rand() % 100;

  //Setting sequence number
  packet.seq = seq;
  //Sending SYN
  send_without_data(packet.seq, 2, sockfd, servaddr);

  //SYN Sent state loop
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

    time_stamp();
    printf("Receiver: %d\n", packet_received.flags);

    //No response
    if(response < 0){
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      time_stamp();
      printf("Timeout\n");
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
        send_without_data(packet.seq, 2, sockfd, servaddr);
        continue;
    }

    if( ! error_check(response, full_packet)){
      //Failed error check, sending NACK
      time_stamp();
      printf("Error check 1\n");
      send_without_data(packet.seq, 8, sockfd, servaddr);
      response = -1;
      continue;
    }

    //Packet does not have the expected flag
    if(packet_received.flags != 3){
      response = -1;
      time_stamp();
      printf("Received faulty message, sending SYN\n");
      send_without_data(packet.seq, 2, sockfd, servaddr);
    }
  }

  //Resets timeouts and sends an ACK
  reset_variables(&nr_of_timeouts, sockfd, &tv);
  time_stamp();
  printf("SYN + ACK received\nConnection Established\n"); 
  send_without_data(packet.seq, 1, sockfd, servaddr);

  //Sets timeout
  tv.tv_sec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
    perror("Error\n");
  }

  return seq;
}