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
#include "../shared/induce_errors.h"
#include "../shared/constants.h"  
#include "receiver_sliding_window.h"
#include "receiver_connection_setup.h"
#include "../shared/utilities.h"
#include "receiver_teardown.h"


//ACK: 1 SYN: 2 FIN: 4 NACK: 8

// int send_nack(base_packet packet, int sockfd, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){
//     char* buf = (char*)&packet; 
//     return send_with_error(sockfd, buf, size, flags, addr, addr_len); 
// }


// Driver code 
int main() { 
    srand(time(0));
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

    //Connection setup function
    while(sender_seq == -1){
        sender_seq = connection_setup(sockfd, cliaddr);
        if(sender_seq == -1){
            printf("Connection reset\n");
        }
    }

    receiver_sliding_window(sockfd, cliaddr, sender_seq);
    //while(sender_seq == -1){
    //    sender_seq = connection_setup(sockfd, cliaddr);
    //}
    //printf("SEQ NUM: %d\n", sender_seq);

    //start_sliding_window(sockfd, cliaddr, sender_seq);
   //void connection_teardown(int sockfd, struct sockaddr_in cliaddr, int seqNr);
    
    return 0; 
} 