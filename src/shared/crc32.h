#ifndef CRC32_H
#define CRC32_H

#include <stdbool.h>
#include "base_packet.h"
#define CRC_DATA_SIZE sizeof(base_packet)

typedef struct {
  char data[CRC_DATA_SIZE]; // Set same size as struct windowPacket
  unsigned int crc;
} crc_packet;

crc_packet create_crc(char data[CRC_DATA_SIZE]); // Replace with struct 

base_packet extract_base_packet(crc_packet packet);

bool valid_crc(crc_packet packet);



#endif