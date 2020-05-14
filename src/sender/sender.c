#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <time.h>
#include "../shared/base_packet.h"
#include "../shared/crc32.h"
#include "../shared/induce_errors.h"
#include "../shared/constants.h"
#include "../shared/utilities.h"
#include "sender_connection_setup.h"
#include "sender_teardown.h"
#include "sender_sliding_window.h"

//ACK: 1 SYN: 2 FIN: 4 NACK: 8

// Driver code 
int main() { 
    srand(time(0));
    int sockfd; 
    struct sockaddr_in servaddr; //, cliaddr; 
    int seq = -1;

  
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

    

    while(seq == -1){
      seq = connection_setup(sockfd, servaddr);
      if(seq == -1){
            printf("Connection reset\n");
        }
    }

    sender_sliding_window(sockfd, servaddr, seq);

    connection_teardown(sockfd, servaddr, 1);

        // Test loop for sending data packets.

    // char message[64];
    // // int i = 4;
    // while(1){
    //   int seq;
    //   printf("Enter SEQ: ");
    //   scanf("%d", &seq);
    //   while(getchar() != '\n');
    //   printf("Enter message: ");
    //   fgets(message, 64, stdin);
    //   message[strlen(message)-1] = '\0';
    //   base_packet packet;
    //   packet.seq = seq;
    //   memcpy(packet.data, message, 64);
    //   crc_packet crcpacket;
    //   crcpacket = create_crc((char*)&packet);
    //   send_with_error(sockfd, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &servaddr, sizeof(servaddr));

    //   // i = i + 8;

    // }
  
    
    return 0; 
} 
