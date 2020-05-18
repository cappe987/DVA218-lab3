#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/utilities.h"
#include "../../include/shared/crc32.h"

ssize_t send_with_error(int sockfd, const void* buf, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){

  if(ERRORS_ENABLED){ // ERRORS_ENABLED can be found in induce_errors.h
    // Do printf whenever an error occurs.

    int error = rand() % 10;

    switch (error)
    {
    case 3: case 4:
      //Packet lost
      time_stamp();
      printf("Packet was lost\n");
      return sizeof(crc_packet);
      break;
    case 5: case 6:
      //Corrupt packet
      time_stamp();
      printf("Packet was corrupt\n");
      char* corrupt_packet = (char*)buf;
      corrupt_packet[10] = 't';
      
      return sendto(sockfd, corrupt_packet, size, flags, addr, addr_len);
      break;
    default:
      //No packet errors
      return sendto(sockfd, buf, size, flags, addr, addr_len);
      break;
    }

  }
  else{
    return sendto(sockfd, buf, size, flags, addr, addr_len); 
  }

}