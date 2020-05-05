typedef struct{
  int seq; // 4 bytes
  int ack; // 4 bytes 
  unsigned char flags; // ACK, NACK, SYN, FIN // 1 byte
  char data[64]; // ? bytes
} base_packet;