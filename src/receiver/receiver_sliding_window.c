#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h> 
// #include <sys/types.h> 
// #include <arpa/inet.h> 
#include <netinet/in.h> 
#include "../shared/base_packet.h"
#include "../shared/crc32.h"
#include "../shared/utilities.h"

#define WINDOW_SIZE 64


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

void reset_window(base_packet window[WINDOW_SIZE]){
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }
}

int find_cumulative(int back, int front, int SEQ, base_packet window[WINDOW_SIZE]){
  // int prev = window[back];
  int prev = -1;
  for(int i = back; i != front; i = (i + 1) % WINDOW_SIZE){
    if(window[i].seq == -1){
      if(prev == -1){
        return SEQ;
      }
      else{
        return prev;
      }
    }
    prev = window[i].seq;
  }
  return prev;
}

void start_sliding_window(int sockfd, struct sockaddr_in cliaddr, int SEQ){ 
  printf(">>> Sliding window started\n");

  int windowBack  = 0;
  int windowFront = -1;

  base_packet window[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
    // printf("%d\n", window[i].seq);
  }

  char buffer[sizeof(crc_packet)]; 
  int read;
  socklen_t len = sizeof(cliaddr);

  while(true){

    read = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet),  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len); 

    crc_packet full_packet = *(crc_packet*) buffer;
    base_packet packet = extract_base_packet(full_packet);
    if( ! error_check(read, full_packet)){
      // Send NACK
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      printf(">>> Failed CRC check\n");
      continue; // Restart loop
    }

    printf(">>> Passed CRC check\n");

    // Packet passed error check

    // Do sequence number check
    int res = sequence_check(windowFront, windowBack, window, packet, SEQ);

    // ACK is sent no matter what the sequence check yields.
    int cumulative = find_cumulative(windowBack, windowFront, SEQ, window);
    send_without_data(cumulative, 1, sockfd, cliaddr);

    if(res < 0){
      // // Send ACK?
      printf(">>> Sequence check failed\n");
    }
    else{
      // // Send ACK
      windowFront = res;

      // Move Back forward if it's received.
      // Consume data that is ready
      // while(window[windowBack].seq != -1 && window[windowBack+1].seq != -1){
      while(window[windowBack].seq != -1){
        // printf(">>>> Moved back forward one step\n");
        printf("%s", window[windowBack].data);
        if(windowBack != windowFront){
          window[windowBack].seq = -1;
          windowBack = (windowBack + 1) % WINDOW_SIZE;
        }
        else{
          SEQ = window[windowBack].seq + 1;
          windowBack = 0;
          windowFront = -1;
          reset_window(window);
        }
      }
    }
    printf("\n");

  }

  // base_packet packet_received = *(base_packet*) buffer;
  // printf("Client : %d\n", packet_received.ack);

  // char* message_to_send = (char*)&packet; 
  // sendto(sockfd, (const char *)message_to_send, sizeof(base_packet),  
  //     MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
  //         len); 
  // printf("Message sent.\n");  

}