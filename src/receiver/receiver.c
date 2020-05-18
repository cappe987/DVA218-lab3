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
#include "../../include/shared/base_packet.h"
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/constants.h"  
#include "../../include/receiver/receiver_sliding_window.h"
#include "../../include/receiver/receiver_connection_setup.h"
#include "../../include/receiver/receiver_teardown.h"
#include "../../include/shared/utilities.h"
#include "../../include/receiver/receiver_teardown.h"

// Driver code 
int main() { 
    srand(time(0));
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
    int sender_seq = -1;
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        time_stamp(); 
        perror("socket creation failed\n"); 
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
        time_stamp();
        perror("bind failed\n"); 
        exit(EXIT_FAILURE); 
    }

    while(sender_seq == -1){
        sender_seq = connection_setup(sockfd,cliaddr);

        sender_seq = receiver_sliding_window(sockfd, cliaddr, sender_seq);

        if(sender_seq == -1){
            time_stamp();
            printf("Connection to receiver lost, restarting setup\n");
        }
        else{
            time_stamp();
            printf("Connection teardown initiated\n"); 
            connection_teardown(sockfd, servaddr, sender_seq);
            return 0;
        }
    }

    return 0; 
} 