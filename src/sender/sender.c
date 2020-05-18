// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: sender                                       #
// # Function: Main program for the sending side, starts the #
// # connection setup, sliding window and teardown functions.#
// ###########################################################


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
#include "../../include/shared/crc32.h"
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/constants.h"
#include "../../include/shared/utilities.h"
#include "../../include/sender/sender_connection_setup.h"
#include "../../include/sender/sender_teardown.h"
#include "../../include/sender/sender_sliding_window.h"

//ACK: 1 SYN: 2 FIN: 4 NACK: 8

// Driver code 
int main() { 
    srand(time(0));
    int sockfd; 
    struct sockaddr_in servaddr; //, cliaddr; 
    int seq = -1;

    
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        time_stamp(); 
        perror("socket creation failed\n"); 
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
        time_stamp();
        printf("Connection reset\n");
        }

        seq = sender_sliding_window(sockfd, servaddr, seq);

        if(seq == -1){
            time_stamp();
           printf("Connection to receiver lost, restarting setup\n");
        }
        else{
            time_stamp();
            printf("Connection teardown initiated\n"); 
            connection_teardown(sockfd, servaddr, seq);
            return 0;
        }
    } 
    
    return 0; 
} 
