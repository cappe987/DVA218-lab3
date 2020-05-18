// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: receiver_connection_setup                    #
// # Function: Handles all the code for the connection setup #
// # for the receiving side. Sends ACKs NACKs and other      #
// # messages to the sender.                                 #
// ###########################################################


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

  len = sizeof(cliaddr);
  packet.seq = 1;
  packet.flags = 3;
  int response = -1, nr_of_timeouts = 0, sender_seq = -1;
  
  //Listen state loop
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

    //No response
    if(response < 0){
      time_stamp();
      printf("Response < 0 \n");
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      time_stamp();
      printf("Timeout\n");
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
        continue;
    }

    
    if( ! error_check(response, full_packet)){
      //Failed error check, sending NACK
      time_stamp();
      printf("Error check 1\n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      response = -1;
      continue;
    }

    time_stamp();
    printf("Sender: %d\n", packet_received.flags);

    //Packet does not have the expected flag
    if(packet_received.flags != 2){
        response = -1;
        time_stamp();
        printf("Received faulty SYN / NACK\n");
        send_without_data(packet.seq, 8, sockfd, cliaddr);   
    }
    //Stores the sequence number for sliding window
    sender_seq = packet_received.seq;
    packet.seq = packet_received.seq;
  }
  
  //Resets timeouts and loops and send a SYN ACK
  reset_variables(&nr_of_timeouts, sockfd, &tv);
  response = -1;
  send_without_data(packet.seq, 3, sockfd, cliaddr);

  //SYN-Received state loop
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

      //No response
     if(response < 0){
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      time_stamp();
        printf("Timeout\n");
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
        send_without_data(packet.seq, 3, sockfd, cliaddr);
        continue;
    }

    if( ! error_check(response, full_packet)){
      //Failed error check, sending NACK
      time_stamp();
      printf("Error check 2\n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      response = -1;
      continue;
    }

    time_stamp();
    printf("Sender: %d\n", packet_received.flags);

    //Packet does not have the expected flag
    if(packet_received.flags != 1){
        response = -1;
        time_stamp();
        printf("Received NACK\n");
        send_without_data(packet.seq, 3, sockfd, cliaddr);
    } 
  }

  //Resets and sets timeouts 
  reset_variables(&nr_of_timeouts, sockfd, &tv);
  tv.tv_sec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
    time_stamp();
    perror("Error\n");
  }

  time_stamp();
  printf("Connection established\n");
  return sender_seq;
}