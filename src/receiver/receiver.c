#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "../shared/base_packet.h"
  
#define PORT     8080 
#define MAXLINE 1024 

//ACK: 1 SYN: 2 FIN: 4 NACK: 8

int connection_setup(int sockfd, struct sockaddr_in cliaddr){
    char buffer[MAXLINE];
  base_packet packet_received;
  base_packet packet;
  packet.seq = 1;
  packet.flags = 3;
  socklen_t len;
  len = sizeof(cliaddr);
  int n;
  char* message_to_send;

  n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &cliaddr, 
                &len); 
    buffer[n] = '\0'; 
  packet_received = *(base_packet*) buffer;
  printf("Sender: %d\n", packet_received.flags);

  if(packet_received.flags != 2){
    packet.flags = 8;
    message_to_send = (char*)&packet; 
    sendto(sockfd, (const char *)message_to_send, sizeof(base_packet),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len); 
    printf("Received faulty SYN, sending NACK.\n");
  }

  packet.flags = 3;
  packet.seq = 2;
  message_to_send = (char*)&packet; 
  sendto(sockfd, (const char *)message_to_send, sizeof(base_packet),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len);  
  printf("SYN + ACK sent.\n");

  n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &cliaddr, 
                &len); 
    buffer[n] = '\0'; 

  packet_received = *(base_packet*) buffer;
  printf("Sender: %d\n", packet_received.flags);

  if(packet_received.flags != 1){
    packet.flags = 8;
    message_to_send = (char*)&packet; 
    sendto(sockfd, (const char *)message_to_send, sizeof(base_packet),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len); 
    printf("Received faulty ACK, sending NACK.\n");
  } 

  printf("Connection established\n");
  return 1;
}

// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    char *hello = "Hello from server"; 
    struct sockaddr_in servaddr, cliaddr; 
    base_packet packet;
    packet.seq = 4;
    packet.ack = 1;
    packet.flags = 3;
    strcpy(packet.data, hello);
    
      
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
      
    int n; 
    socklen_t len;
  
    len = sizeof(cliaddr);  //len is value/result

    int connection = connection_setup(sockfd, cliaddr);

    // n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
    //             MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
    //             &len); 
    // buffer[n] = '\0'; 

    // base_packet packet_received = *(base_packet*) buffer;
    // printf("Client : %d\n", packet_received.flags);

    // char* message_to_send = (char*)&packet; 
    // sendto(sockfd, (const char *)message_to_send, sizeof(base_packet),  
    //     MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
    //         len); 
    // printf("Message sent.\n");  
      
    return 0; 
} 