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

#define WINDOW_SIZE 64

int sock;
struct sockaddr_in socket_addr;

base_packet window[WINDOW_SIZE];
int windowBack = 0;
int windowFront = -1;
int nr_of_timeouts = 0;

pthread_mutex_t winlock;

void* input(void* params){
  int seq = *(int*)params;
  // printf("SENDER WINDOW SEQ: %d\n", seq);
  char message[DATA_SIZE];
  // int i = 4;
  while(1){
    // int MYSEQ;
    // printf("Enter SEQ: ");
    // scanf("%d", &MYSEQ);
    // while(getchar() != '\n');
    printf("Enter message: ");
    fgets(message, DATA_SIZE, stdin);
    message[strlen(message)-1] = '\0';
    base_packet packet;
    packet.seq = seq;
    seq++;
    memcpy(packet.data, message, DATA_SIZE);

    pthread_mutex_lock(&winlock);

    windowFront++;
    window[windowFront] = packet;

    crc_packet crcpacket;
    crcpacket = create_crc((char*)&packet);
    // crcpacket.crc = 50; // Induce error for testing
    // crcpacket.data[0] = 'X';
    send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
    pthread_mutex_unlock(&winlock);
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
    while(window[windowBack].seq <= packet.seq && windowBack != windowFront + 1){
      // printf("WINSEQ: %d\n", window[i].seq);
      printf(">>> SEQ %d ACKED\n", window[windowBack].seq);
      window[windowBack].seq = -1;
      windowBack = (windowBack + 1) % WINDOW_SIZE;
    }

    if(windowBack == windowFront + 1){ // Window is empty.
      windowBack = 0;
      windowFront = -1;
      reset_window(window);
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
  sock = sockfd;
  socket_addr = sockaddr;
  pthread_t input_thread;
  pthread_mutex_init(&winlock, NULL);
  char buffer[sizeof(crc_packet)];
  socklen_t len;

  struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;

  // This is because someone wrote bullshit code. It is not actually used.
  // int bullshit = -1; 
  reset_timeout(&nr_of_timeouts, sockfd, &tv);

  reset_window(window);

  pthread_create(&input_thread, NULL, input, &SEQ);

  // Set some timeout
  int read = 0;
  while(true){
    read = recvfrom(sockfd, buffer, sizeof(crc_packet), 
              MSG_WAITALL, (struct sockaddr*) &sockaddr, 
              &len);

    // printf("READ: %d\n", read);
    if(read < 0){
      // Timeout
      // // This is because someone wrote bullshit code. It is not actually used.
      // int bullshit = -1; 
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
        handle_response(extract_base_packet(full_packet));
      }

      // ACK/NACK received
      

    }

  }




  return 0;
}