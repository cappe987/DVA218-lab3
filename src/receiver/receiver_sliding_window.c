// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: receiver_sliding_window                      #
// # Function: Handles the sliding window for the receiver.  #
// # It consumes the packets when it can, otherwise they are #
// # stored in the window until the correct packets arrive.  # 
// ###########################################################

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


// go-back-N

int waiting_for_resends = 0;

int go_back_n(int front,
              int back,
              base_packet window[WINDOW_SIZE],
              base_packet packet,
              int SEQ){

  // It only needs to check if it's the expected value. 
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
    window[0] = packet;
    return 0;
  }
  else if(packet.seq < SEQ || packet.seq <= window[back].seq){
    return front;
  }
  else if(front == -1){ // packet.seq > SEQ and first packet.
    int index = packet.seq - SEQ;
    if (index < WINDOW_SIZE){
      window[index] = packet;
      return index;
    }
    else{
      // Package too far into the future.
      return -1;
    }
  }
  else{ // packet.seq >= SEQ. Find where it should go.
    if(window[back].seq == -1){

      // No back exists, use SEQ.
      int seq_for_index = SEQ;
      int new_front = false;

      for(int i = 0; i < WINDOW_SIZE; i++){
        if(i == front){new_front = true;}
        if(seq_for_index == packet.seq){
          window[i] = packet;
          return new_front ? i : front;
        }
        seq_for_index++;
      }
      return -1;
    }
    else{

      int seq_for_index = window[back].seq + 1;
      int new_front = false;

      for(int i = (back + 1) % WINDOW_SIZE; i != back; i = (i+1) % WINDOW_SIZE){
        if(i == front){ new_front = true;}
        if(seq_for_index == packet.seq){
          window[i] = packet;
          return new_front ? i : front;
        }
        seq_for_index++;
      }
    }
  }

  return -1;
}

// Clears the window.
void reset_window(base_packet window[WINDOW_SIZE]){
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }
}

// Finds the next expected seq.
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

  int expected_seq = SEQ + 1;
  int window_back  = 0;
  int window_front = -1;
  int nr_of_timeouts = 0;
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  SEQ++;
  printf("------ STARTING SEQ: %d ------\n", SEQ);

  base_packet window[WINDOW_SIZE];
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }

  char buffer[sizeof(crc_packet)]; 
  int read;
  socklen_t len = sizeof(cliaddr);

  // Connection loop
  while(true){

    read = recvfrom(sockfd, (char *)buffer, sizeof(crc_packet),  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len); 

    crc_packet full_packet = *(crc_packet*) buffer;
    base_packet packet = extract_base_packet(full_packet);
    
    //No response, timeout.
    if(read < 0){
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
        if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
            return -1;
        }
        continue;
    }

    // Packet successfully received.
    reset_timeout(&nr_of_timeouts, sockfd, &tv);

    // CRC check
    if( ! error_check(read, full_packet)){
      time_stamp();
      printf("Failed CRC check\n");
      // Send NACK
      send_without_data(packet.seq, 8, sockfd, cliaddr);
      printf("\n");
      continue; // Restart loop
    }

    // Packet passed error check

    if(packet.flags == 4){
      // Fin
      return packet.seq;
    }
    if(packet.flags > 0){
      // What? Flags should only be able to be 4.
      // Ignore packet.
      continue;
    }

    int res = -1;

    time_stamp();
    printf("Received seq %d\n", packet.seq);
    // Do sequence number check
    if(USING_SELECTIVE_REPEAT){ // Selective repeat
      res = selective_repeat(window_front, window_back, window, packet, SEQ);

      // ACK is sent no matter what the sequence check yields.
      int cumulative = find_cumulative(window_back, window_front, expected_seq, window);
      send_without_data(cumulative, 1, sockfd, cliaddr);
    }
    else { // go-back-n
      res = go_back_n(window_front, window_back, window, packet, SEQ);
      if(res < 0 && waiting_for_resends == 0){ // Go back
        // Send ACK to go back.
        send_without_data(SEQ, 1, sockfd, cliaddr);
        waiting_for_resends = 1;
      }
      else if(res >= 0 && waiting_for_resends == 1){ // Resends found
        // Send ACK for next seq.
        send_without_data(SEQ + 1, 1, sockfd, cliaddr);
        waiting_for_resends = 0;
      }
      else if(res >= 0){ // Message received, just send ACK.
        // Send ACK for next seq.
        send_without_data(SEQ + 1, 1, sockfd, cliaddr);

      }
    }

    if(res < 0){
      // Sliding window returned < 0.
      // Already handled previously.
    }
    else{
      window_front = res;

      // Move window_back forward if it's received.
      // Consume data that is ready.
      // 
      while(window[window_back].seq != -1){
        time_stamp();
        printf("Seq %d consumed. Data: %s\n", window[window_back].seq, window[window_back].data);
        expected_seq = window[window_back].seq + 1;
        if(window_back != window_front){
          // SEQ = window[window_back].seq;
          window[window_back].seq = -1;
          window_back = (window_back + 1) % WINDOW_SIZE;
        }
        else{
          // Window completely empty. Reset variables.
          // Probably not necessary, but it makes things easier.
          SEQ = window[window_back].seq + 1;
          window_back = 0;
          window_front = -1;
          reset_window(window);
        }
      }
    }
    printf("\n");

  }
}