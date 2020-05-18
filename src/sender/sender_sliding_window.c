// ###########################################################
// #         This program is written and designed by         #
// #   Alexander Andersson, Casper Andersson, Nick Grannas   #
// #           During the period 6/5/20 - 26/5/20            #
// #          For course DVA 218 Datakommunikation           #
// ###########################################################
// #                      Description                        #
// # File name: sender_sliding_window                        #
// # Function: Handles the sliding window for the sender.    #
// # It will fill the sliding window and if it is full it    #
// # will fill its buffer instead until that is full.        #
// # It will send the packets when it can.                   #
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
#include "../shared/base_packet.h"
#include "../shared/crc32.h"
#include "../shared/utilities.h"
#include "../shared/constants.h"
#include "../shared/induce_errors.h"
#include <semaphore.h>

// #define WINDOW_SIZE 4

//Must be below WINDOW_SIZE
#define WIN_MAX_SIZE 16

int sock;
struct sockaddr_in socket_addr;

base_packet window[WINDOW_SIZE];
int windowBack = 0;
int windowFront = -1;
int buffer_front = -1;
int nr_of_timeouts = 0;
int packets_in_window = 0;
int last_SEQ = 0;
int current_SEQ = 0;

pthread_mutex_t winlock;
pthread_mutex_t acklock;
sem_t empty;

int back_front_diff(int back, int front){
  if(back <= front){
    return front - back;
  }
  else{
    return (WINDOW_SIZE - back) + front;
  }
}

void* input(void* params){
  int seq = *(int*)params;
  // printf("SENDER WINDOW SEQ: %d\n", seq);
  char message[DATA_SIZE];
  // int i = 4;

  if(APPLICATION_SIMULATOR){
    int packets_to_send;
    int nr_of_packets;
    char letter;
    int loops = 0;
    last_SEQ = seq;
    while(loops != NUMBER_OF_LOOPS){
      
      nr_of_packets = 0;
      packets_to_send = rand() % PACKETS_TO_SEND;
      last_SEQ = last_SEQ + packets_to_send;
      letter = '0';
      printf("Number of packets to send: %d\n", packets_to_send);

      //Sends all the amount of packets that were generated
      while(nr_of_packets !=  packets_to_send){
        
        message[0] = letter;
        message[1] = '\0';

        base_packet packet;
        packet.seq = seq;
        // printf("Last SEQ: %d Current SEQ: %d\n", last_SEQ, current_SEQ);
        seq++;
        
        memcpy(packet.data, message, DATA_SIZE);

        //Waiting for slot in sliding buffer if its full
        sem_wait(&empty);
        pthread_mutex_lock(&winlock);

        buffer_front = (buffer_front + 1) % WINDOW_SIZE;
        window[buffer_front] = packet;
        packets_in_window++;

        //Checks the sliding window, if its full we dont send
        if(back_front_diff(windowBack, windowFront) < WIN_MAX_SIZE - 1 || windowFront == -1){
          windowFront = (windowFront + 1) % WINDOW_SIZE;
          crc_packet crcpacket;
          crcpacket = create_crc((char*)&packet);
          // crcpacket.crc = 50; // Induce error for testing
          // crcpacket.data[0] = 'X';
          send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));

        }
        else{
          printf("Didn't send %d\n", packet.seq);
        }
        nr_of_packets++;
        letter++;
        
        pthread_mutex_unlock(&winlock);
      }
      loops++;
    }
    
    //Waiting for the last ACK to open the lock
    pthread_mutex_lock(&acklock);
    pthread_mutex_unlock(&acklock);
    printf("Last ACK received\n");
    return NULL;
    
  }
  else{
    while(1){
      // int MYSEQ;
      // printf("Enter SEQ: ");
      // scanf("%d", &MYSEQ);
      // while(getchar() != '\n');
      printf("Enter message for SEQ %d: ", seq);
      fgets(message, DATA_SIZE, stdin);
      message[strlen(message)-1] = '\0';
      base_packet packet;
      packet.seq = seq;
      seq++;
      if(strcmp("QUIT", message) == 0){
        packet.flags = 4;
      }
      memcpy(packet.data, message, DATA_SIZE);

      pthread_mutex_lock(&winlock);

      if(packets_in_window == WINDOW_SIZE - 1){
        printf("Window full\n");
        seq--;
        pthread_mutex_unlock(&winlock);
        continue;
      }
      buffer_front = (buffer_front + 1) % WINDOW_SIZE;
      window[buffer_front] = packet;
      packets_in_window++;

      if(back_front_diff(windowBack, windowFront) < WIN_MAX_SIZE - 1 || windowFront == -1){
        windowFront = (windowFront + 1) % WINDOW_SIZE;
        crc_packet crcpacket;
        crcpacket = create_crc((char*)&packet);
        // crcpacket.crc = 50; // Induce error for testing
        // crcpacket.data[0] = 'X';
        send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));

      }
      else{
        printf("Didn't send %d\n", packet.seq);
      }
      pthread_mutex_unlock(&winlock);
    }
  }
  return NULL;
}

void send_more(base_packet window[WINDOW_SIZE]){
  while(windowFront != buffer_front){
    windowFront = (windowFront + 1) % WINDOW_SIZE;
    crc_packet crcpacket;
    crcpacket = create_crc((char*)&window[windowFront]);
    // crcpacket.crc = 50; // Induce error for testing
    // crcpacket.data[0] = 'X';
    printf("Sending packet %d\n", window[windowFront].seq);
    send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
    if( ! (back_front_diff(windowBack, windowFront) < WIN_MAX_SIZE - 1)){
      return;
    }

  }
}


void reset_window(base_packet window[WINDOW_SIZE]){
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }
}

void handle_response(base_packet packet){
  int flag = packet.flags;
  // printf("FLAG: %d\n", flag);
  if(flag % 2 == 1){ // ACK
    pthread_mutex_lock(&winlock);
    // for(int i = 0; i <= windowFront; i++){
    //   printf("WIN: %d\n", window[i].seq);
    // }
    // int i = windowBack;
    // while(window[i].seq != -1 && window[i].seq <= packet.seq){
    while(window[windowBack].seq < packet.seq && windowBack != windowFront + 1){
      // printf("WINSEQ: %d\n", window[i].seq);
      printf(">>> SEQ %d ACKED\n", window[windowBack].seq);
      current_SEQ++;
      if(current_SEQ == last_SEQ){
        printf("Finished looking for ACKs \n");
        pthread_mutex_unlock(&acklock);
        return;
      }
      window[windowBack].seq = -1;
      windowBack = (windowBack + 1) % WINDOW_SIZE;
      sem_post(&empty);
    }

    // printf("Back: %d, Front: %d, BufferFront: %d\n", windowBack, windowFront, buffer_front);
    if(windowBack == buffer_front + 1 && windowFront == buffer_front){ // Window is empty.
      windowBack = 0;
      windowFront = -1;
      buffer_front = -1;
      reset_window(window);
    }
    else{
      send_more(window);
    }
    pthread_mutex_unlock(&winlock);
    printf(">>> Next expected is %d\n", packet.seq);

    flag = flag - 1;
  }
  if(flag % 4 == 2){ // SYN
    // Not expected here
    flag = flag - 2;
  }
  if(flag % 8 == 4){ // FIN 
    // End connection
  
    flag = flag - 4;
  }
  if(flag % 16 == 8){ // NACK
    printf(">>> NACK received for seq %d\n", packet.seq);
    // Send again
    pthread_mutex_lock(&winlock);
    for(int i = windowBack; i != windowFront + 1; i = (i+1) % WINDOW_SIZE){
      if(window[i].seq == packet.seq){
        printf(">>> Resending seq %d\n", window[i].seq);
        crc_packet full_packet = create_crc((char*)&window[i]);
        send_with_error(sock, (const char*)&full_packet, sizeof(crc_packet), 
            MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
      }
    }
    pthread_mutex_unlock(&winlock);
  }
}


void resend_all(base_packet window[WINDOW_SIZE]){
  pthread_mutex_lock(&winlock);
  crc_packet full_packet;
  // printf(">>> Resending ALL\n");
  for(int i = windowBack; i != windowFront + 1; i = (i + 1) % WINDOW_SIZE){
    if(window[i].seq != -1){
      printf(">>> Resending seq %d\n", window[i].seq);
      full_packet = create_crc((char*)&window[i]);
      send_with_error(sock, (const char*)&full_packet, sizeof(crc_packet), 
          MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
    }
  }
  pthread_mutex_unlock(&winlock);
}


int sender_sliding_window(int sockfd, struct sockaddr_in sockaddr, int SEQ){
  printf(">>> Sliding window started\n");
  sock = sockfd;
  socket_addr = sockaddr;
  pthread_t input_thread;
  pthread_mutex_init(&winlock, NULL);
  pthread_mutex_init(&acklock, NULL);
  pthread_mutex_lock(&acklock);
  sem_init(&empty, 0, WINDOW_SIZE - 1);
  char buffer[sizeof(crc_packet)];
  socklen_t len;
  int setup_SEQ = SEQ;
  SEQ++;
  printf("------ STARTING SEQ: %d ------\n", SEQ);
  current_SEQ = SEQ;
  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  reset_timeout(&nr_of_timeouts, sockfd, &tv);

  reset_window(window);

  pthread_create(&input_thread, NULL, input, &SEQ);

  // Set some timeout
  int read = 0;
  while(last_SEQ != current_SEQ){
    read = recvfrom(sockfd, buffer, sizeof(crc_packet), 
              MSG_WAITALL, (struct sockaddr*) &sockaddr, 
              &len);

    // printf("READ: %d\n", read);
    if(read < 0){
      // Timeout
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
        pthread_cancel(input_thread);
        return -1;
      }
      resend_all(window);
    }
    else{
      crc_packet full_packet = *(crc_packet*)buffer;
      int res = error_check(read, full_packet);
      reset_timeout(&nr_of_timeouts, sockfd, &tv);
      if( ! res){
        // CRC failed
      }
      else{
        // reset_timeout(&nr_of_timeouts, sockfd, &tv);
        // CRC passed
        // int result = handle_response(extract_base_packet(full_packet));
        // handle_response(extract_base_packet(full_packet));
        base_packet packet = extract_base_packet(full_packet);
        if(packet.seq == setup_SEQ && packet.flags == 3){
          send_without_data(packet.seq, 1, sockfd, sockaddr);
        }
        else{
          handle_response(packet);
        }
      }
    }
  }
  return last_SEQ;
}