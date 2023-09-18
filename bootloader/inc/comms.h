#ifndef COMMS_H
#define COMMS_H

#include "common-defines.h"

#define PACKET_DATA_LENGTH (16)
#define PACKET_LENGTH_BYTES (1)
#define PACKET_CRC_BYTES (1)
#define PACKET_LENGTH (PACKET_LENGTH_BYTES + PACKET_DATA_LENGTH + PACKET_CRC_BYTES)

#define BL_PACKET_SYNC_OBSERVED_DATA0 (0x20)
#define BL_PACKET_FW_UPDATE_REQ_DATA0 (0x31)
#define BL_PACKET_FW_UPDATE_RES_DATA0 (0x37)
#define BL_PACKET_DEVICE_ID_REQ_DATA0 (0x3C)
#define BL_PACKET_DEVICE_ID_RES_DATA0 (0x3F)
#define BL_PACKET_FW_LENGTH_REQ_DATA0 (0x42)
#define BL_PACKET_FW_LENGTH_RES_DATA0 (0x45)
#define BL_PACKET_READY_FOR_DATA_DATA0 (0x48)
#define BL_PACKET_UPDATE_SUCCESSFUL_DATA0 (0x54)
#define BL_PACKET_NACK_DATA0 (0x59)


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
bool comms_is_single_byte_packet(comms_packet_t* pkt, uint8_t byte);
void comms_create_single_byte_packet(comms_packet_t* pkt, uint8_t byte);

#endif
