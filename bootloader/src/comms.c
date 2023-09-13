#include "comms.h"
#include "core/uart.h"
#include "core/crc8.h"
// #include <assert.h>

#define PACKET_BUFFER_LENGTH (8)
static comms_packet_t packet_buffer[PACKET_BUFFER_LENGTH];
static uint32_t packet_read_index = 0;
static uint32_t packet_write_index = 0;
static uint32_t packet_buffer_mask = PACKET_BUFFER_LENGTH - 1;

typedef enum comms_state_t {
	CommsState_Length,
	CommsState_Data,
	CommsState_CRC
} comms_state_t;

uint8_t comms_compute_crc(comms_packet_t* pkt) {
	return crc8((uint8_t*)pkt, PACKET_LENGTH - PACKET_CRC_BYTES);
}

static bool comms_is_single_byte_packet(comms_packet_t* pkt, uint8_t byte) {
	if (pkt->length != 1) return false;
	if (pkt->data[0] != byte) return false;
	for (uint8_t i=1;i<PACKET_DATA_LENGTH;i++) {
		if (pkt->data[i] != 0xff) return false;
	}
	return true;
}

#define PACKET_RETX_DATA0 (0x19)
static comms_packet_t retx_pkt = { .length=1, .data={0xff}, .crc=0 };
static void retx_pkt_init(void) {
	retx_pkt.data[0] = PACKET_RETX_DATA0;
	retx_pkt.crc = comms_compute_crc(&retx_pkt);
}

#define PACKET_ACK_DATA0 (0x15)
static comms_packet_t ack_pkt = { .length=1, .data={0xff}, .crc=0 };
static void ack_pkt_init(void) {
	ack_pkt.data[0] = PACKET_ACK_DATA0;
	ack_pkt.crc = comms_compute_crc(&ack_pkt);
}

static void comms_packet_copy(const comms_packet_t* src, comms_packet_t* dst) {
	dst->length = src->length;
	for (uint8_t i=0;i<PACKET_DATA_LENGTH;i++) {
		dst->data[i] = src->data[i];
	}
	dst->crc = src->crc;
}

void comms_setup(void) {
	retx_pkt_init();
	ack_pkt_init();
}

static comms_state_t state = CommsState_Length;
static uint8_t data_byte_count = 0U;
static comms_packet_t tmp_pkt = { .length=0, .data={0}, .crc=0 };
static comms_packet_t last_transmitted_pkt = { .length=0, .data={0}, .crc=0 };
// comms_update is to update the firmware.
void comms_update(void) {
	while (uart_data_available()) {
		switch (state) {
			case CommsState_Length: {
				tmp_pkt.length = uart_read_byte();
				state = CommsState_Data;
			} break;
			case CommsState_Data: {
				tmp_pkt.data[data_byte_count++] = uart_read_byte();
				if (data_byte_count >= PACKET_DATA_LENGTH) {
					data_byte_count = 0;
				}
				state = CommsState_CRC;
			} break;
			case CommsState_CRC: {
				tmp_pkt.crc = uart_read_byte();
				if (tmp_pkt.crc != comms_compute_crc(&tmp_pkt)) {
					comms_write(&retx_pkt);
					state = CommsState_Length;
					break;
				}

				if (comms_is_single_byte_packet(&tmp_pkt, PACKET_RETX_DATA0)) {
					comms_write(&last_transmitted_pkt);
					state = CommsState_Length;
					break;
				}

				if (comms_is_single_byte_packet(&tmp_pkt, PACKET_ACK_DATA0)) {
					state = CommsState_Length;
					break;
				}

				uint32_t next_write_index = (packet_write_index + 1) & packet_buffer_mask;
				// assert(next_write_index != packet_read_index);
				if (next_write_index == packet_read_index) {
					__asm__("BKPT #0");
				}

				comms_packet_copy(&tmp_pkt, &packet_buffer[packet_write_index]);
				packet_write_index = (packet_write_index + 1) & packet_buffer_mask;
				comms_write(&ack_pkt);
				state = CommsState_Length;

			} break;
			default: {
				state = CommsState_Length;
			}
		}
	}
}

bool comms_packets_available(void) {
	return packet_read_index != packet_write_index;
}

void comms_write(comms_packet_t* pkt) {
	uart_write((uint8_t*)pkt, PACKET_LENGTH);
	comms_packet_copy(pkt, &last_transmitted_pkt);
}

void comms_read(comms_packet_t* pkt) {
	comms_packet_copy(&packet_buffer[packet_read_index],pkt);
	packet_read_index = (packet_read_index + 1) & packet_buffer_mask;
}
