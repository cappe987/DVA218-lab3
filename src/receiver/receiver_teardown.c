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
    
    printf("Receiver: connection teardown initialized.\n\n");
    int response = -1, sequanceNumber = seqNr+1,timeouts = 0;
    base_packet received_packet;
    char buffer[sizeof(crc_packet)];  
    socklen_t len = sizeof(cliaddr);

    //Reset socket timeout timer
    struct timeval tv = { TIMEOUT, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


    //Testing snippet
    //Just for testing lol
    //---------------------------------
    while(response < 0)
    {
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    }
    printf("FIN got\n");
    response=-1;
    printf("--------\n");

    //---------------------------------


    send_without_data(sequanceNumber, 5, sockfd, cliaddr);
    printf("--------\n");
    while(response < 0){
        sleep(1);
        response = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet), MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
        //Check for timeout
        if (response < 0)
        { 
            increment_timeout(&timeouts, sockfd, &tv);
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
            printf("--------\n");   
            continue;
        }
        crc_packet full_packet = *(crc_packet*) buffer;
        received_packet = extract_base_packet(full_packet);
        //Check for faculty package
        if(!error_check(response, full_packet)){
            // Send NACK
            printf("Got corrupt packet\n");
            send_without_data(sequanceNumber + 1, 8, sockfd, cliaddr);           
            printf("--------\n");
            response=-1;
            continue;
        }
        if(received_packet.flags == 8) {
            printf("NACK received, Resend FIN + ACK\n");
            send_without_data(sequanceNumber, 5, sockfd, cliaddr);
            printf("--------\n");
            response=-1;        
            continue;
        }

        if(received_packet.seq != sequanceNumber +1) {
            if (received_packet.seq = sequanceNumber -1 && received_packet.flags == 4)
            {
                send_without_data(sequanceNumber, 5, sockfd, cliaddr);
                printf("--------\n");
            }
            else{
                printf("Got seq number: %d. Expected seq number: %d\n", received_packet.seq, sequanceNumber+1);
                response = -1;
                printf("--------\n");
                continue;
            }

        }
    
        printf("ACK got (SEQ %d)\n",received_packet.seq);
        printf("Receiver: connection teardown complete.\n");
    
        close(sockfd);
        return;
        
    }
}