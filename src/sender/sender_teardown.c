#include <stdio.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/time.h> 
#include "../shared/base_packet.h"
#include "../shared/induce_errors.h"
#include "../shared/constants.h"  
#include "../shared/utilities.h"
#include "sender_connection_setup.h"

int connection_teardown(int sockfd, struct sockaddr_in servaddr, int sequence){   
    
    printf("Sender: connection teardown initialized.\n\n");
    int response = -1, seq = sequence+1, nr_of_timeouts = 0;
    char buffer[sizeof(crc_packet)]; 
    base_packet packet_received;
    socklen_t len;

    //Reset socket timeout timer
    struct timeval tv = { TIMEOUT, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    send_without_data(seq, 4, sockfd, servaddr);  
    printf("--------\n");

    while(response < 0){
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
        if (response < 0)
        {
            increment_timeout(&nr_of_timeouts, sockfd, &tv);
            //Check for lost connection
            if(nr_of_timeouts >= NR_OF_TIMEOUTS_ALLOWED)
            {
                printf("Max amount of timeouts reached. Shuting down\n");
                close(sockfd);
                return 1;
            }
            //Resend FIN
            send_without_data(seq, 4, sockfd, servaddr);        
            printf("FIN resent %d times.\n", nr_of_timeouts);
            printf("--------\n");   
            continue;
        }
        crc_packet full_packet = *(crc_packet*) buffer;
        packet_received = extract_base_packet(full_packet);
        
        if(!error_check(response, full_packet)){
            // Send NACK
            sleep(1);
            printf("Got corrupt packet\n");
            send_without_data(seq + 1, 8, sockfd, servaddr);
            printf("--------\n");
            response=-1;
            continue;
        }

        if(packet_received.flags == 8){      
            printf("Received NACK, FIN resent.\n");
            send_without_data(seq, 4, sockfd, servaddr);
            printf("--------\n");
            response=-1;        
            continue;
        }

        if(packet_received.seq != seq +1) {
            printf("Got seq number: %d. Expected seq number: %d", packet_received.seq, seq+1);
            response = -1;
            continue;
        }

    }
    printf("FIN + ACK got (SEQ %d)\n",packet_received.seq);
    printf("--------\n");
    seq+=2;  
    send_without_data(seq, 1, sockfd, servaddr); 
  
  
    struct timeval tv2 = { 5, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(struct timeval));
    while(response > -1){
        response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
        if (response > 0)
        {
            printf("Received NACK, resent ACK");
            send_without_data(seq, 1, sockfd, servaddr);
            printf("--------\n");
        }
    
    }
    printf("Sender: connection teardown complete.\n");
    close(sockfd); 
    return 1;
}