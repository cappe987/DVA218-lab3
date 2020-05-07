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

//ACK: 1 SYN: 2 FIN: 4 NACK: 8

int connection_teardown(int sockfd, struct sockaddr_in servaddr, struct timeval tv, base_packet packet_received, base_packet packet, char* buffer, socklen_t len, int sequence){
  int response = -1, seq = sequence, nr_of_timeouts = 0;
  char* message_to_rec;

  seq++;
  packet.flags = 4;
  packet.seq = seq;
  message_to_rec = (char*)&packet;     
  send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  printf("FIN sent.\n");

  while(response < 0){
      response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
      buffer[response] = '\0'; 
      packet_received = *(base_packet*) buffer;
      printf("Server : %d\n", packet_received.flags); 

      if(packet_received.flags != 5){
        message_to_rec = (char*)&packet;       
        send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
        printf("Received NACK or timeout, FIN resent.\n");
        increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            close(sockfd);
            return 1;
          }
      }
    }

  reset_variables(&nr_of_timeouts, &response, &tv);

  seq++;
  packet.flags = 1;
  packet.seq = seq;
  message_to_rec = (char*)&packet;     
  send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  printf("ACK sent.\n");

  sleep(10);

  close(sockfd); 
  return 1;
}


int connection_setup(int sockfd, struct sockaddr_in servaddr, struct timeval tv, base_packet packet_received, base_packet packet, char* buffer, socklen_t len){
  int response = -1, seq = -1, nr_of_timeouts = 0;
  char* message_to_rec;

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
    buffer[response] = '\0'; 
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

// Driver code 
int main() { 
    int sockfd; 
    char buffer[DATA_SIZE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 
    int seq = -1;
    base_packet packet;
    base_packet packet_received;
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    //socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
    {
      perror("Error");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 

    socklen_t len;

    while(seq == -1){
      seq = connection_setup(sockfd, servaddr, tv, packet_received, packet, buffer, len);
    }

    connection_teardown(sockfd, servaddr, tv, packet_received, packet, buffer, len, seq);

    // char* message_to_rec = (char*)&packet; 
    // sendto(sockfd, (const char *)message_to_rec, sizeof(base_packet),  
    //     MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
    //         sizeof(servaddr)); 
    // printf("Hello message sent.\n"); 
          
    // n = recvfrom(sockfd, (char *)buffer, DATA_SIZE,  
    //             MSG_WAITALL, (struct sockaddr *) &servaddr, 
    //             &len); 
    // buffer[n] = '\0'; 

    // base_packet packet_received = *(base_packet*) buffer;
    // printf("Server : %d\n", packet_received.seq); 
  
    
    return 0; 
} 
