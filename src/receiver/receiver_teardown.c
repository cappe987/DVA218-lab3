#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/time.h> 
#include <stdio.h>
#include <unistd.h> 

#include "../shared/utilities.h"
#include "../shared/base_packet.h"
#include "../shared/constants.h"
#include "../shared/crc32.h" 


void connection_teardown(int sockfd, struct sockaddr_in cliaddr, int seqNr){
    
    printf("Receiver: connection teardown initialized.\n");
    int response = -1, sequanceNumber = seqNr+1,timeouts = -1;
    base_packet received_packet;
    char buffer[sizeof(crc_packet)];  
    socklen_t len = sizeof(cliaddr);

    //Reset socket timeout timer
    struct timeval tv = { TIMEOUT/2, 0 };
    increment_timeout(&timeouts, sockfd, &tv);
    response = -1;
    //Send FIN+ACK

    //Testing snippet
    //---------------------------------
    while(response < 0)
    {
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    }
    printf("FIN got\n");
    response=-1;


    //---------------------------------


    send_without_data(sequanceNumber, 5, sockfd, cliaddr);
    printf("FIN + ACK sent.\n");
 
    while(response < 0){
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
        //Check for timeout
        if (response < 0)
        {
            
            increment_timeout(&timeouts, sockfd, &tv);
            response = -1;
            //Check for lost connection
            if(timeouts >= NR_OF_TIMEOUTS_ALLOWED)
            {
                printf("Max amount of timeouts reached. Shuting down\n");
                close(sockfd);
                return;
            }
            //Resend FIN+ACK
            send_without_data(sequanceNumber, 5, sockfd, cliaddr);        
            printf("FIN + ACK resent %d times.\n", timeouts);   
            continue;
        }
        printf("Hello!\n");
        crc_packet full_packet = *(crc_packet*) buffer;
        received_packet = extract_base_packet(full_packet);
        printf("Got packet!\n");
        //Check for faculty package
        if(!error_check(response, full_packet)){
            // Send NACK
            send_without_data(received_packet.seq, 8, sockfd, cliaddr);
            printf("'Waiting for last ack' state failed CRC check\n");
            continue;
        }
        else
        {
            printf("Receiver: connection teardown complete.\n");
            close(sockfd);
            return;
        }
    }
}

