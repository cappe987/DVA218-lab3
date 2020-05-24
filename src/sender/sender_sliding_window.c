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
#include "../../include/shared/base_packet.h"
#include "../../include/shared/crc32.h"
#include "../../include/shared/utilities.h"
#include "../../include/shared/settings.h"
#include "../../include/shared/induce_errors.h"
#include <semaphore.h>



int sock;
struct sockaddr_in socket_addr;

base_packet window[WINDOW_SIZE];
int window_back = 0;
int window_front = -1;
int buffer_front = -1;
int nr_of_timeouts = 0;
int packets_in_window = 0;
int last_SEQ = 0;
int current_SEQ = 0;
int previous_cumulative = -1;

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

// Function for inputting data, mainly automated.
void* input(void* params){
  int seq = *(int*)params;
  char message[DATA_SIZE];

  if(APPLICATION_SIMULATOR){
    int packets_to_send;
    int nr_of_packets;
    char letter;
    int loops = 0;
    last_SEQ = seq;
    while(loops != NUMBER_OF_LOOPS){
      
      nr_of_packets = 0;
      packets_to_send = (rand() % PACKETS_TO_SEND) + 3;
      last_SEQ = last_SEQ + packets_to_send;
      letter = '0';
      printf("Number of packets to send: %d\n", packets_to_send);

      //Sends all the amount of packets that were generated
      while(nr_of_packets !=  packets_to_send){
        
        message[0] = letter;
        message[1] = '\0';

        base_packet packet;
        packet.seq = seq;
        seq++;
        
        memcpy(packet.data, message, DATA_SIZE);

        //Waiting for slot in sliding buffer if its full
        sem_wait(&empty);
        pthread_mutex_lock(&winlock);

        buffer_front = (buffer_front + 1) % WINDOW_SIZE;
        window[buffer_front] = packet;
        packets_in_window++;

        //Checks the sliding window, if its full we dont send
        if(back_front_diff(window_back, window_front) < WIN_MAX_SIZE - 1 || window_front == -1){
          window_front = (window_front + 1) % WINDOW_SIZE;
          crc_packet crcpacket;
          crcpacket = create_crc((char*)&packet);
          // crcpacket.crc = 50; // Induce error for testing
          // crcpacket.data[0] = 'X';
          send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));

        }
        else{
          // printf("Didn't send %d\n", packet.seq);
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
    time_stamp();
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

      if(back_front_diff(window_back, window_front) < WIN_MAX_SIZE - 1 || window_front == -1){
        window_front = (window_front + 1) % WINDOW_SIZE;
        crc_packet crcpacket;
        crcpacket = create_crc((char*)&packet);
        send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));

      }
      else{
        // printf("Didn't send %d\n", packet.seq);
      }
      pthread_mutex_unlock(&winlock);
    }
  }
  return NULL;
}

// ACK's have been received. Send more packets if there exists any in buffer.
void send_more(base_packet window[WINDOW_SIZE]){
  if(back_front_diff(window_back, window_front) >= WIN_MAX_SIZE - 2 && window_front != window_back - 1){
    return;
  }
  while(window_front != buffer_front){
    window_front = (window_front + 1) % WINDOW_SIZE;
    crc_packet crcpacket;
    crcpacket = create_crc((char*)&window[window_front]);
    // crcpacket.crc = 50; // Induce error for testing
    // crcpacket.data[0] = 'X';
    time_stamp();
    printf("Sending packet %d\n", window[window_front].seq);
    send_with_error(sock, (const char*)&crcpacket, sizeof(crc_packet), MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
    if(back_front_diff(window_back, window_front) >= WIN_MAX_SIZE - 2){
      return;
    }

  }
}


// Sender reached timeout. Resend all packets that haven't been ACKed.
void resend_all(base_packet window[WINDOW_SIZE], char* reason){
  crc_packet full_packet;
  for(int i = window_back; i != (window_front + 1) % WINDOW_SIZE; i = (i + 1) % WINDOW_SIZE){
    if(window[i].seq != -1){
      time_stamp();
      printf("Resending seq %d due to %s\n", window[i].seq, reason);
      full_packet = create_crc((char*)&window[i]);
      send_with_error(sock, (const char*)&full_packet, sizeof(crc_packet), 
          MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
    }
  }
}



// Clears the window
void reset_window(base_packet window[WINDOW_SIZE]){
  for(int i = 0; i < WINDOW_SIZE; i++){
    window[i].seq = -1;
  }
}

void handle_response(base_packet packet){
  int flag = packet.flags;
  if(flag % 2 == 1){ // ACK
    pthread_mutex_lock(&winlock);

    if(USING_SELECTIVE_REPEAT){
      if(packet.seq == previous_cumulative){ // Resend due to cumulative
        for(int i = window_back; i != (window_front + 1) % WINDOW_SIZE; i = (i+1) % WINDOW_SIZE){
          if(window[i].seq == packet.seq){
            time_stamp();
            printf("Resending seq %d due to cumulative ACK\n", window[i].seq);
            crc_packet full_packet = create_crc((char*)&window[i]);
            send_with_error(sock, (const char*)&full_packet, sizeof(crc_packet), 
                MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
          }
        }
      }
    }
    else{
      if(packet.seq == previous_cumulative){
        resend_all(window, "go-back-N cumulative ACK");
      }
    }

    previous_cumulative = packet.seq;

    while(window[window_back].seq < packet.seq && window_back != window_front + 1){
      time_stamp();
      printf("SEQ %d ACKED\n", window[window_back].seq);
      if(current_SEQ == last_SEQ){
        time_stamp();
        printf("Finished looking for ACKs \n");
        pthread_mutex_unlock(&acklock);
        return;
      }
      current_SEQ++;
      window[window_back].seq = -1;
      window_back = (window_back + 1) % WINDOW_SIZE;
      sem_post(&empty);
    }

    if(window_back == buffer_front + 1 && window_front == buffer_front){ // Window is empty.
      window_back = 0;
      window_front = -1;
      buffer_front = -1;
      reset_window(window);
    }
    else{
      send_more(window);
    }
    pthread_mutex_unlock(&winlock);
    time_stamp();
    printf("Next expected is %d\n", packet.seq);

    flag = flag - 1;
  }
  if(flag % 4 == 2){ // SYN
    // Not expected here
    flag = flag - 2;
  }
  if(flag % 8 == 4){ // FIN 
    // End connection
    // Receiver isn't expected to end the connection.
  
    flag = flag - 4;
  }
  if(flag % 16 == 8){ // NACK
    time_stamp();
    printf("NACK received for seq %d\n", packet.seq);
    // Send again
    pthread_mutex_lock(&winlock);
    for(int i = window_back; i != (window_front + 1) % WINDOW_SIZE; i = (i+1) % WINDOW_SIZE){
      if(window[i].seq == packet.seq){
        time_stamp();
        printf("Resending seq %d due to NACK\n", window[i].seq);
        crc_packet full_packet = create_crc((char*)&window[i]);
        send_with_error(sock, (const char*)&full_packet, sizeof(crc_packet), 
            MSG_CONFIRM, (const struct sockaddr*) &socket_addr, sizeof(socket_addr));
      }
    }
    pthread_mutex_unlock(&winlock);
  }
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

    if(read < 0){
      // Timeout
      increment_timeout(&nr_of_timeouts, sockfd, &tv);
      if(nr_of_timeouts == NR_OF_TIMEOUTS_ALLOWED){
        // Other side assumed to be lost. Exit connection.
        pthread_cancel(input_thread);
        return -1;
      }
      pthread_mutex_lock(&winlock);
      resend_all(window, "timeout");
      pthread_mutex_unlock(&winlock);
    }
    else{
      // Packet received.
      crc_packet full_packet = *(crc_packet*)buffer;
      int res = error_check(read, full_packet);
      reset_timeout(&nr_of_timeouts, sockfd, &tv);
      if( ! res){
        // CRC failed
        time_stamp();
        printf("Failed CRC check\n");
      }
      else{
        // CRC passed
        base_packet packet = extract_base_packet(full_packet);
        if(packet.seq == setup_SEQ && packet.flags == 3){
          // In case Receiver Setup hasn't received its last ACK.
          send_without_data(packet.seq, 1, sockfd, sockaddr);
        }
        else{
          handle_response(packet);
        }
      }
    }
    printf("\n");
  }
  return last_SEQ;
}