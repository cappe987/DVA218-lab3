// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: Utilities                                    #
// # Function: Here are the functions that are used globaly  #
// # in the program.                                         #
// ###########################################################


#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/time.h> 
#include <unistd.h>
#include <time.h>
#include <netinet/in.h> 
#include "../../include/shared/utilities.h"
#include "../../include/shared/settings.h"
#include "../../include/shared/base_packet.h"
#include "../../include/shared/induce_errors.h"
#include "../../include/shared/crc32.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void time_stamp(){
  
  time_t rawtime = time(NULL);
  struct timeval tv;

  gettimeofday(&tv, NULL);

  //Removes the unnecessary micro seconds 
  int msec = tv.tv_usec % 1000000;
    
  if (rawtime == -1){
    printf("Get raw time failed\n");
    return;
  }

  //Gets the time and puts it into a struct  
  struct tm *time = localtime(&rawtime);
    
  if (time == NULL){
    printf("Get local time failed\n");
    return;
  }
  
  //Prints the timestamp message for all prints
  printf("[%02d:%02d:%02d.%03d]: ", time->tm_hour, time->tm_min, time->tm_sec, msec);
  
  return;
}

//Resets the timeout checks for each state
void reset_timeout(int *timeout, int sockfd, struct timeval *tv){
    *timeout = 0;
    tv->tv_sec = TIMEOUT;
    struct timeval temp = *tv;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
      printf("Error setting the sock option\n");
  }
}

//Increments the timeout for each state
void increment_timeout(int *timeout, int sockfd, struct timeval *tv){
  (*timeout)++;
  tv->tv_sec = tv->tv_sec * 2;
  struct timeval temp = *tv;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    printf("Error setting the sock option\n");
  }
}

//Function is used for sending flags only
ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr){
  socklen_t len = sizeof(sockaddr);
  base_packet packet;
  packet.data[0] = '\0';
  packet.seq = seq;
  packet.flags = flag;
  time_stamp();
  printf("Sent ");
  if(flag % 2  == 1){printf("ACK "); flag = flag - 1;}
  if(flag % 4  == 2){printf("SYN "); flag = flag - 2;}
  if(flag % 8  == 4){printf("FIN "); flag = flag - 4;}
  if(flag % 16 == 8){printf("NACK ");}

  printf("(SEQ %d)\n",seq);

  crc_packet full_packet = create_crc((char*)&packet);
  return send_with_error(sockfd, (const char *)&full_packet, sizeof(crc_packet),  
      MSG_CONFIRM, (const struct sockaddr *)&sockaddr, len); 

}

//Checks the crc package
int error_check(int read, crc_packet packet){
  if(read == 0){ // Socket has shut down, not sure if needed
    time_stamp();
    printf("Socket closed unexpectedly\n");
    exit(1);
    // return false;
  }
  else if(read < 0){
    // printf(">>> Error on recvfrom |%s|\n", strerror(errno));
  }
  else { // Successful read
    return valid_crc(packet);
  }
  return false;
}

