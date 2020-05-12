#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/time.h> 
#include <netinet/in.h> 
#include "utilities.h"
#include "constants.h"
#include "base_packet.h"
#include "induce_errors.h"
#include "crc32.h"
#include <errno.h>


void reset_variables(int *timeout, int *response, struct timeval *tv){
    *timeout = 0;
    *response = -1;
    tv->tv_sec = TIMEOUT;
}

void increment_timeout(int *timeout, int *response, int sockfd, struct timeval *tv){
  (*timeout)++;
  tv->tv_sec = tv->tv_sec * 2;
  *response = -1;
  struct timeval temp = *tv;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &temp, sizeof(temp))< 0){
    perror("Error");
  }
}

ssize_t send_without_data(int seq, int flag, int sockfd, struct sockaddr_in sockaddr){
  socklen_t len = 0;
  base_packet packet;
  packet.data[0] = '\0';
  packet.seq = seq;
  packet.flags = flag;
  printf("Sent ");
  if(flag == 1){printf("ACK"); flag = flag - 1;}
  if(flag == 2){printf("SYN"); flag = flag - 2;}
  if(flag == 4){printf("FIN"); flag = flag - 4;}
  if(flag == 8){printf("NACK");}

  printf("\n");

  crc_packet full_packet = create_crc((char*)&packet);
  return send_with_error(sockfd, (const char *)&full_packet, sizeof(crc_packet),  
      MSG_CONFIRM, (const struct sockaddr *) &sockaddr, len); 

}

int error_check(int read, crc_packet packet){
  if(read == 0){ // Socket has shut down, not sure if needed
    printf(">>> Socket closed for some reason\n");
    exit(1);
    // return false;
  }
  else if(read < 0){
    printf(">>> Error on recvfrom |%s|\n", strerror(errno));
  }
  else { // Successful read
    return valid_crc(packet);
  }
  return false;
}

// Selective repeat
// Returns negative number if failed.
// Otherwise returns new "front"
int sequence_check(int front, 
                   int back, 
                   base_packet window[WINDOW_SIZE], 
                   base_packet packet, 
                   int SEQ){

  if(front == -1 && packet.seq == SEQ){
    printf(">>> First packet is SEQ\n");
    window[0] = packet;
    return 0;
  }
  else if(packet.seq < SEQ || packet.seq <= window[back].seq){
    // Send ACK.
    return front;
  }
  else if(front == -1){ // packet.seq > SEQ and first packet.
    int index = packet.seq - SEQ;
    if (index < WINDOW_SIZE){
      printf(">>> First package fit in window on index %d\n", index);
      window[index] = packet;
      return index;
    }
    else{
      // Package too far into the future.
      printf(">>> Packet too far in the future\n");
      return -1;
    }
  }
  else{ // packet.seq >= SEQ. Find where it should go.
    printf("packet.seq >= SEQ\n");
    if(window[back].seq == -1){
      printf("No back exists yet\n");

      // No back exists, use SEQ.
      int seqForIndex = SEQ;
      int newfront = false;

      for(int i = 0; i < WINDOW_SIZE; i++){
        if(i == front){newfront = true;}
        if(seqForIndex == packet.seq){
          window[i] = packet;
          printf(">>> No back - found spot in Window on index %d\n", i);
          return newfront ? i : front;
        }
        seqForIndex++;
      }
      printf(">>> No back. Packet too far in the future\n");
      return -1;
    }
    else{
      printf("Back exists - try to find slot\n");

      int seqForIndex = window[back].seq + 1;
      int newfront = false;

      for(int i = (back + 1) % WINDOW_SIZE; i != back; i = (i+1) % WINDOW_SIZE){
        if(i == front){ newfront = true;}
        if(seqForIndex == packet.seq){
          window[i] = packet;
          printf(">>> Found slot for packet on index %d\n", i);
          return newfront ? i : front;
        }
        seqForIndex++;
      }
    }
  }

  printf(">>> No slot found\n");

  return -1;
}