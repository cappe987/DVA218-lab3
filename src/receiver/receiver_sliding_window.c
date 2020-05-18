#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h> 
// #include <sys/types.h> 
// #include <arpa/inet.h> 
#include <netinet/in.h> 
#include "../../include/shared/base_packet.h"
#include "../../include/shared/crc32.h"
#include "../../include/shared/utilities.h"
#include "../../include/shared/constants.h"

#define USING_SELECTIVE_REPEAT 1

// go-back-N

int waiting_for_resends = 0;

int go_back_n(int front,
              int back,
              base_packet window[WINDOW_SIZE],
              base_packet packet,
              int SEQ){

  if(packet.seq == SEQ){
    window[back] = packet;
    return front + 1;
  }
  return -1;
}

// Selective repeat
// Returns negative number if failed.
// Otherwise returns new "front"
int selective_repeat(int front, 
                   int back, 
                   base_packet window[WINDOW_SIZE], 
                   base_packet packet, 
                   int SEQ){

  if(front == -1 && packet.seq == SEQ){
    // printf(">>> First packet is SEQ\n");
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
      // printf(">>> First package fit in window on index %d\n", index);
      window[index] = packet;
      return index;
    }
    else{
      // Package too far into the future.
      // printf(">>> Packet too far in the future\n");
      return -1;
    }
  }
  else{ // packet.seq >= SEQ. Find where it should go.
    // printf("packet.seq >= SEQ\n");
    if(window[back].seq == -1){
      // printf("No back exists yet\n");

      // No back exists, use SEQ.
      int seqForIndex = SEQ;
      int newfront = false;

      for(int i = 0; i < WINDOW_SIZE; i++){
        if(i == front){newfront = true;}
        if(seqForIndex == packet.seq){
          window[i] = packet;
          // printf(">>> No back - found spot in Window on index %d\n", i);
          return newfront ? i : front;
        }
        seqForIndex++;
      }
      // printf(">>> No back. Packet too far in the future\n");
      return -1;
    }
    else{
      // printf("Back exists - try to find slot\n");

      int seqForIndex = window[back].seq + 1;
      int newfront = false;

      for(int i = (back + 1) % WINDOW_SIZE; i != back; i = (i+1) % WINDOW_SIZE){
        if(i == front){ newfront = true;}
        if(seqForIndex == packet.seq){
          window[i] = packet;
          // printf(">>> Found slot for packet on index %d\n", i);
          return newfront ? i : front;
        }
        seqForIndex++;
      }
    }
  }

  // printf(">>> No slot found\n");

  return -1;
}

void reset_window(base_packet window[WINDOW_SIZE]){
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }
}

int find_cumulative(int back, int front, int SEQ, base_packet window[WINDOW_SIZE]){
  int expects = SEQ;
  int i = back;
  while(window[i].seq != -1){
    expects = window[i].seq + 1;
    i = (i+1) % WINDOW_SIZE;
  }
  return expects;
}

int receiver_sliding_window(int sockfd, struct sockaddr_in cliaddr, int SEQ){ 
  printf(">>> Sliding window started\n");
  if(USING_SELECTIVE_REPEAT){
    printf("Using selective repeat\n");
  }
  else{
    printf("Using go-back-n\n");
  }

  int expectedSEQ = SEQ + 1;
  int windowBack  = 0;
  int windowFront = -1;
  int nr_of_timeouts = 0;
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  SEQ++;
  printf("------ STARTING SEQ: %d ------\n", SEQ);

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
    
    //No response
    if(read < 0){
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
        continue;
    }

    reset_timeout(&nr_of_timeouts, sockfd, &tv);

    if( ! error_check(read, full_packet)){
      time_stamp();
      printf("Failed CRC check\n");
      // Send NACK
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      printf("\n");
      continue; // Restart loop
    }

    // printf(">>> Passed CRC check\n");

    // Packet passed error check

    if(packet.flags == 4){
      // Fin
      // Should it finish waiting for all packets and then exit?
      return packet.seq;
    }
    // if(packet.flags > 0){
    //   // What? Flags should only be able to be 4.
    //   continue;
    // }

    int res = -1;

    time_stamp();
    printf("Received seq %d\n", packet.seq);
    // Do sequence number check
    if(USING_SELECTIVE_REPEAT){ // Selective repeat
      res = selective_repeat(windowFront, windowBack, window, packet, SEQ);

      // ACK is sent no matter what the sequence check yields.
      // if(res >= 0){printf(">>> Seq %d placed in sliding window\n", packet.seq);}
      int cumulative = find_cumulative(windowBack, windowFront, expectedSEQ, window);
      // printf(">>> Send ACK for seq %d, cumulative: %d ---- ", packet.seq, cumulative);
      send_without_data(cumulative, 1, sockfd, cliaddr);
    }
    else { // go-back-n
      res = go_back_n(windowFront, windowBack, window, packet, SEQ);
      if(res < 0 && waiting_for_resends == 0){ // Go back
        send_without_data(SEQ, 1, sockfd, cliaddr);
        waiting_for_resends = 1;
      }
      else if(res >= 0 && waiting_for_resends == 1){ // Resends found
        // printf(">>> Seq %d placed in sliding window\n", packet.seq);
        send_without_data(SEQ + 1, 1, sockfd, cliaddr);
        waiting_for_resends = 0;
      }
      else if(res >= 0){ // Message received, just send ACK.
        // printf(">>> Seq %d placed in sliding window\n", packet.seq);
        send_without_data(SEQ + 1, 1, sockfd, cliaddr);

      }
    }

    if(res < 0){
      // // Send ACK?
      // printf(">>> Sequence check failed %d\n", packet.seq);
    }
    else{
      // printf(">>> Seq %d placed in sliding window\n", packet.seq);
      // // Send ACK
      windowFront = res;

      // Move Back forward if it's received.
      // Consume data that is ready
      // while(window[windowBack].seq != -1 && window[windowBack+1].seq != -1){
      while(window[windowBack].seq != -1){
        // printf(">>>> Moved back forward one step\n");
        time_stamp();
        printf("Seq %d consumed. Data: %s\n", window[windowBack].seq, window[windowBack].data);
        expectedSEQ = window[windowBack].seq + 1;
        if(windowBack != windowFront){
          // SEQ = window[windowBack].seq;
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
}