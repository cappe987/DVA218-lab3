#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/time.h> 
#include "../shared/base_packet.h"
#include "../shared/induce_errors.h"
#include "../shared/constants.h"  
#include "../shared/shared_functions.h"
#include "receiver_sliding_window.h"


//ACK: 1 SYN: 2 FIN: 4 NACK: 8

// int send_nack(base_packet packet, int sockfd, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){
//     char* buf = (char*)&packet; 
//     return send_with_error(sockfd, buf, size, flags, addr, addr_len); 
// }


int connection_setup(int sockfd, struct sockaddr_in cliaddr){
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
  len = sizeof(cliaddr);  //len is value/result
  packet.seq = 1;
  packet.flags = 3;
  int response = -1, nr_of_timeouts = 0, sender_seq = -1;
  char* message_to_send;
#include "receiver_sliding_window.h"
  
  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    //printf("Response %d\n", response);
    packet_received = *(base_packet*) buffer;
    printf("Sender: %d\n", packet_received.flags);

    if(packet_received.flags != 2){
        packet.flags = 8;
        message_to_send = (char*)&packet; 
        send_with_error(sockfd, (const char *)message_to_send, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
        printf("Received faulty SYN / NACK or timeout, sending NACK.\n");

        increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
        printf("TIMEOUT: %ld\n", tv.tv_sec);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
    }
    sender_seq = packet_received.seq;
  }

  reset_variables(&nr_of_timeouts, &response, &tv);

  packet.flags = 3;
  sender_seq++;
  packet.seq = sender_seq;
  message_to_send = (char*)&packet; 
  send_with_error(sockfd, (const char *)message_to_send, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);  
  printf("SYN + ACK sent.\n");

  while(response < 0){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
    packet_received = *(base_packet*) buffer;
    printf("Sender: %d\n", packet_received.flags);

    if(packet_received.flags != 1){
        message_to_send = (char*)&packet; 
        send_with_error(sockfd, (const char *)message_to_send, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
        
        printf("Received NACK or timeout, resending SYN + ACK.\n");
        increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
    } 
    sender_seq = packet_received.seq;
  }

  reset_variables(&nr_of_timeouts, &response, &tv);

  printf("Connection established\n");
  return sender_seq;
}

// Driver code 
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
    int sender_seq = -1;
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    while(sender_seq == -1){
        sender_seq = connection_setup(sockfd, cliaddr);
    }
    printf("SEQ NUM: %d\n", sender_seq);

    start_sliding_window(sockfd, cliaddr, sender_seq);
    
    return 0; 
} 