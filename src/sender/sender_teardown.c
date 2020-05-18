// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: sender_teardown                              #
// # Function: Handles all the code for the teardown of the  #
// # sending side. Sends the neccessary messages to the      #
// # receiving side to teardown the connection.              #
// ###########################################################

#include <stdio.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/time.h> 
#include "../../include/shared/base_packet.h"
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/constants.h"  
#include "../../include/shared/utilities.h"
// #include "../../include/sender/sender_connection_setup.h"

int connection_teardown(int sockfd, struct sockaddr_in servaddr, int sequence){   
    
    time_stamp();
    printf("Sender: connection teardown initialized.\n");
    int response = -1, seq = sequence+1, nr_of_timeouts = 0;
    char buffer[sizeof(crc_packet)]; 
    base_packet packet_received;
    socklen_t len;

    //Reset socket timeout timer
    struct timeval tv = { TIMEOUT, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    send_without_data(seq, 4, sockfd, servaddr);  

    while(response < 0){
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
        if (response < 0)
        {
            increment_timeout(&nr_of_timeouts, sockfd, &tv);
            //Check for lost connection
            if(nr_of_timeouts >= NR_OF_TIMEOUTS_ALLOWED)
            {
                time_stamp();
                printf("Max amount of timeouts reached. Shuting down\n");
                close(sockfd);
                return 1;
            }
            //Resend FIN
            send_without_data(seq, 4, sockfd, servaddr);
            time_stamp();        
            printf("FIN resent %d times.\n", nr_of_timeouts);  
            continue;
        }
        crc_packet full_packet = *(crc_packet*) buffer;
        packet_received = extract_base_packet(full_packet);
        
        if(!error_check(response, full_packet)){
            // Send NACK
            sleep(1);
            time_stamp();
            printf("Failed CRC check\n");
            send_without_data(seq + 1, 8, sockfd, servaddr);
            response=-1;
            continue;
        }

        if(packet_received.flags == 8){ 
            time_stamp();     
            printf("Received NACK, FIN resent.\n");
            send_without_data(seq, 4, sockfd, servaddr);
            response=-1;        
            continue;
        }

        if(packet_received.seq != seq +1) {
            time_stamp();
            printf("Got seq number: %d. Expected seq number: %d\n", packet_received.seq, seq+1);
            response = -1;
            continue;
        }

    }

    time_stamp();
    printf("FIN + ACK got (SEQ %d)\n",packet_received.seq);
    seq+=2;  
    send_without_data(seq, 1, sockfd, servaddr); 
  
  
    struct timeval tv2 = { 5, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(struct timeval));
    while(response > -1){
        response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
        if (response > 0)
        {
            time_stamp();
            printf("Received NACK, resent ACK");
            send_without_data(seq, 1, sockfd, servaddr);
        }
    
    }

    time_stamp();
    printf("Sender: connection teardown complete.\n");
    close(sockfd); 
    return 1;
}