#ifndef COMMS_H
#define COMMS_H

#include "common-defines.h"

#define PACKET_DATA_LENGTH (16)
#define PACKET_LENGTH_BYTES (1)
#define PACKET_CRC_BYTES (1)
#define PACKET_LENGTH (PACKET_LENGTH_BYTES + PACKET_DATA_LENGTH + PACKET_CRC_BYTES)

typedef struct comms_packet_t {
	uint8_t length;
	uint8_t data[PACKET_DATA_LENGTH];
	uint8_t crc;
} comms_packet_t;

void comms_setup(void);

// comms_update is to update the firmware.
void comms_update(void);

bool comms_packets_available(void);
void comms_write(comms_packet_t* pkt);
void comms_read(comms_packet_t* pkt);
uint8_t comms_compute_crc(comms_packet_t* pkt);

#endif
