#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include "induce_errors.h"
#include "crc32.h"

ssize_t send_with_error(int sockfd, const void* buf, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){

  if(ERRORS_ENABLED){ // ERRORS_ENABLED can be found in induce_errors.h
    // Do printf whenever an error occurs.

    srand(time(0));
    int error = rand() % 7;
    printf("error: %d\n", error);

    switch (error)
    {
    case 3: case 4:
      //Packet lost
      printf("Packet was lost\n");
      return sizeof(crc_packet);
      break;
    case 5: case 6:
      //Corrupt packet
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