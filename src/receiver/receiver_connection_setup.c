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
#include "../../include/shared/base_packet.h"
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/constants.h"  
#include "../../include/shared/utilities.h"
#include "../../include/receiver/receiver_connection_setup.h"
#include "../../include/shared/crc32.h"

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
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0){
    time_stamp();
    printf("Error setting the sock option\n");
    return -1;
  }

  //Initialize variables for the first packet
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
      printf("No response from sender \n");
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
      printf("Failed CRC check \n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      response = -1;
      continue;
    }

    //if the packet does not have the expected flag it will be a corrupted or NACK packet
    if(packet_received.flags != 2){
        response = -1;
        time_stamp();
        printf("Received faulty SYN or NACK\n");
        send_without_data(packet.seq, 8, sockfd, cliaddr);   
    }
    //Stores the sequence number for sliding window
    sender_seq = packet_received.seq;
    packet.seq = packet_received.seq;
  }
  
  //Resets the timeouts and loops then sends a SYN + ACK
  reset_timeout(&nr_of_timeouts, sockfd, &tv);
  response = -1;
  send_without_data(packet.seq, 3, sockfd, cliaddr);

  //SYN-Received state loop
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    full_packet = *(crc_packet*) buffer;
    packet_received = extract_base_packet(full_packet);

      //No response
    if(response < 0){
      time_stamp();
      printf("No response from the sender\n");
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      time_stamp();
      printf("Timeout\n");
      if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
        return -1;
      }
      //Resend the SYN + ACK
      send_without_data(packet.seq, 3, sockfd, cliaddr);
      continue;
    }

    if( ! error_check(response, full_packet)){
      //Failed error check, sending NACK
      time_stamp();
      printf("Failed CRC check\n");
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      response = -1;
      continue;
    }

    //If the packet does not have the expected flag it will be and NACK
    if(packet_received.flags != 1){
        response = -1;
        time_stamp();
        printf("Did not receive ACK\n");
        //Resend SYN + ACK
        send_without_data(packet.seq, 3, sockfd, cliaddr);
    } 
  }

  //Resets and sets timeouts 
  reset_timeout(&nr_of_timeouts, sockfd, &tv);
  tv.tv_sec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0){
    time_stamp();
    printf("Error setting the sock option\n");
  }

  time_stamp();
  printf("Connection established\n");
  return sender_seq;
}