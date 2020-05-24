#ifndef CONSTANTS_H
#define CONSTANTS_H

// The port the socket operates on.
#define PORT 8080 

// The size of the data array in the packet.
#define DATA_SIZE 64 

// Timeout starting time (seconds). This doubles for each timeout in a row.
#define TIMEOUT 2

// How many timeouts are allowed before assuming the connection is gone.
#define NR_OF_TIMEOUTS_ALLOWED 5

// Automatically send data or use manual input. Manual input probably does not work any more.
#define APPLICATION_SIMULATOR 1

// Number of waves to send data in.
#define NUMBER_OF_LOOPS 1

// Amout of packets to send. It randomizes a number (rand() % PACKETS_TO_SEND) + 3
#define PACKETS_TO_SEND 7

// Set to 1 to use selecive repeat, 0 to use go-back-N.
#define USING_SELECTIVE_REPEAT 1

// Size of the array/buffer that the sliding window operates on
#define WINDOW_SIZE 64

//Must be below WINDOW_SIZE
#define WIN_MAX_SIZE 16

// Toggle the error generator.
#define ERRORS_ENABLED 0


#endif