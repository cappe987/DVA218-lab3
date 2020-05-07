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
#include "../shared/utilitys.h"

//ACK: 1 SYN: 2 FIN: 4 NACK: 8

// Driver code 
int main() { 
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
    }

    // connection_teardown(sockfd, servaddr, seq);
  
    
    return 0; 
} 
