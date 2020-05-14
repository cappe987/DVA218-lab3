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
    
    int response = -1, seq = sequence, nr_of_timeouts = 0;
    char* message_to_rec;
    char buffer[sizeof(crc_packet)]; 
    base_packet packet;
    base_packet packet_received;
    socklen_t len;
    //socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    printf("Sender: connection teardown initialized.\n");

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))< 0)
    {
      perror("Error");
    }

    seq++;

  send_without_data(seq, 4, sockfd, servaddr);     
  printf("FIN sent.\n");

  while(response < 0){
      response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
      if (response < 0)
      {
            increment_timeout(&nr_of_timeouts, &response, sockfd, &tv);
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
            continue;
      }
      printf("Got packet1!\n");
      crc_packet full_packet = *(crc_packet*) buffer;
      packet_received = extract_base_packet(full_packet);
      printf("Got packet2!\n");

      if(packet_received.flags != 5){
        message_to_rec = (char*)&packet;       
        send_with_error(sockfd, (const char *)message_to_rec, sizeof(base_packet), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)); 
        printf("Received NACK or timeout, FIN resent.\n");
        
        increment_timeout(&nr_of_timeouts, sockfd, &tv);
        response = -1;
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            close(sockfd);
            return 1;
          }
      }
    }

  // reset_variables(&nr_of_timeouts, &response, &tv);

  seq++;
  packet.flags = 1;
  packet.seq = seq;
  message_to_rec = (char*)&packet;     
  send_without_data(seq, 1, sockfd, servaddr); 
  printf("ACK sent.\n");

  //sleep(10);
  printf("Sender: connection teardown complete.\n");
  close(sockfd); 
  return 1;
}