// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: induce_errors                                #
// # Function: All messages sent that has data go through    #
// # here. If error simulation is enables it will choose     #
// # randomly what type of error the packet will receive.    #
// # The packets can receive no errors, become lost or       #
// # become corrupt.                                         #
// ###########################################################

#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <string.h>
#include <errno.h>
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/utilities.h"
#include "../../include/shared/crc32.h"
#include "../../include/shared/settings.h"

ssize_t send_with_error(int sockfd, const void* buf, size_t size, int flags, const struct sockaddr* addr, socklen_t addr_len){

  if(ERRORS_ENABLED){ // ERRORS_ENABLED can be found in induce_errors.h
    // Do printf whenever an error occurs.

    int error = rand() % 14;
    int res = 0;

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
      
      res = sendto(sockfd, corrupt_packet, size, flags, addr, addr_len);
      if (res < 0){
        printf("ERROR: %s\n", strerror(errno));
      }
      return res;
      break;
    default:
      //No packet errors
      res = sendto(sockfd, buf, size, flags, addr, addr_len);
      if (res < 0){
        printf("ERROR: %s\n", strerror(errno));
      }
      return res;
      break;
    }

  }
  else{
    return sendto(sockfd, buf, size, flags, addr, addr_len); 
  }

}