#include <sys/socket.h>
#include "../shared/constants.h" 
#include "../shared/base_packet.h"
#include <arpa/inet.h> 




void connection_teardown(int sockfd, struct sockaddr_in cliaddr, socklen_t len, int seqNr){
    base_packet packet, received_packet;
    char buffer[DATA_SIZE];
    packet.flags = 5;
    packet.seq = seqNr + 1;
    char* message_to_send;
    //Maybe needed maybe not, dunno
    //reset_variables(&nr_of_timeouts, &response, &tv);
    message_to_send = (char*)&packet; 
    send_with_error(sockfd, (const char *)message_to_send, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);  
    printf("FIN + ACK sent.\n");

    int response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
    if (response > 0)
    {
        received_packet = *(base_packet*) buffer;
        if(received_packet.flags == 1)
        {
            packet.flags = 1;
            packet.seq = received_packet.seq + 1;
            send_with_error(sockfd, (const char *)message_to_send, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
        }
    }
}

