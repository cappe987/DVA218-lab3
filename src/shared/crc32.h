#ifndef CRC32_H
#define CRC32_H

#include <stdbool.h>
#define CRC_DATA_SIZE 128

typedef struct {
  char data[CRC_DATA_SIZE]; // Set same size as struct windowPacket
  unsigned int crc;
} crc_packet;

crc_packet create_crc(char data[CRC_DATA_SIZE]); // Replace with struct 

bool valid_crc(crc_packet packet);



#endif