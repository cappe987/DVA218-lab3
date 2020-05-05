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

#define PORT     8080 
#define MAXLINE 1024 

//ACK: 1 SYN: 2 FIN: 4 NACK: 8
  
int connection_setup(int sockfd, struct sockaddr_in servaddr){
  char buffer[MAXLINE];
  base_packet packet_received;
  base_packet packet;
  packet.seq = 1;
  packet.flags = 2;
  socklen_t len;
  int n;
  char* message_to_rec;
  
  
  
  //socket timeout thingy
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
  {
     perror("Error");
  }
  

  message_to_rec = (char*)&packet; 
    sendto(sockfd, (const char *)message_to_rec, sizeof(base_packet),  
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
  printf("Connection setup message sent.\n");

  n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 
    buffer[n] = '\0'; 
    packet_received = *(base_packet*) buffer;
    printf("Server : %d\n", packet_received.flags); 

  //Need loop here with timeouts.
  if(packet_received.flags != 3){
    message_to_rec = (char*)&packet; 
        sendto(sockfd, (const char *)message_to_rec, sizeof(base_packet),  
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                sizeof(servaddr)); 
    printf("Connection setup message resent.\n");
  }

  packet.seq = 2;
  packet.flags = 1;
  message_to_rec = (char*)&packet; 
    sendto(sockfd, (const char *)message_to_rec, sizeof(base_packet),  
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
  printf("SYN + ACK received, sending ACK.\nConnection Established\n"); 

  return 1;
}

// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from client"; 
    struct sockaddr_in servaddr, cliaddr; 
    base_packet packet;
    packet.seq = 9;
    packet.ack = 10;
    packet.flags = 'B';
    strcpy(packet.data, hello);
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr));
    
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n; 
    socklen_t len;

    int connection = connection_setup(sockfd, servaddr);

    // char* message_to_rec = (char*)&packet; 
    // sendto(sockfd, (const char *)message_to_rec, sizeof(base_packet),  
    //     MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
    //         sizeof(servaddr)); 
    // printf("Hello message sent.\n"); 
          
    // n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
    //             MSG_WAITALL, (struct sockaddr *) &servaddr, 
    //             &len); 
    // buffer[n] = '\0'; 

    // base_packet packet_received = *(base_packet*) buffer;
    // printf("Server : %d\n", packet_received.seq); 
  
    close(sockfd); 
    return 0; 
} 
