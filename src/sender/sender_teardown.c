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
    char buffer[sizeof(crc_packet)]; 
    base_packet packet_received;
    socklen_t len;
    //socket timeout
    struct timeval tv = { TIMEOUT, 0 };

    printf("Sender: connection teardown initialized.\n");

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


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
      crc_packet full_packet = *(crc_packet*) buffer;
      packet_received = extract_base_packet(full_packet);
      printf("Got: \n");
      int flag = packet_received.flags;
      if(flag % 2  == 1){printf("ACK "); flag = flag - 1;}
      if(flag % 4  == 2){printf("SYN "); flag = flag - 2;}
      if(flag % 8  == 4){printf("FIN "); flag = flag - 4;}
      if(flag % 16 == 8){printf("NACK ");}
      if(packet_received.flags != 5){      
        printf("Received NACK, FIN resent.\n");
        response = -1;
      }
    }

  seq++;  
  send_without_data(seq, 1, sockfd, servaddr); 
  printf("ACK sent.\n");
  
  struct timeval tv2 = { 10, 0 };
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(struct timeval));
  while(response > -1){
    response = recvfrom(sockfd, (char *)buffer, DATA_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len); 
    if (response > 0)
    {
      send_without_data(seq, 1, sockfd, servaddr);
      printf("Received NACK, resent ACK");
    }
    
  }
  printf("Sender: connection teardown complete.\n");
  close(sockfd); 
  return 1;
}