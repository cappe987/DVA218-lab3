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

//ACK: 1 SYN: 2 FIN: 4 NACK: 8
  
int connection_setup(int sockfd, struct sockaddr_in servaddr, struct timeval tv, base_packet packet_received, base_packet packet, char* buffer, socklen_t len){
  packet.seq = 1;
  packet.flags = 2;
  int response = -1, seq = -1;
  char* message_to_rec;

  srand(time(0));
  seq = rand() % 100;
  

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
      response = -1;
      tv.tv_sec = tv.tv_sec * 2;
    }
  }

  tv.tv_sec = TIMEOUT;
  response = -1;

  packet.seq = 2;
  packet.flags = 1;
  message_to_rec = (char*)&packet; 
  send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
  printf("SYN + ACK received, sending ACK.\nConnection Established\n"); 

  return 1;
}

// Driver code 
int main() { 
    int sockfd; 
    char buffer[DATA_SIZE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 
    int connection = 0;
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

    while(connection != 1){
      connection = connection_setup(sockfd, servaddr, tv, packet_received, packet, buffer, len);
    }

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
  
    close(sockfd); 
    return 0; 
} 
