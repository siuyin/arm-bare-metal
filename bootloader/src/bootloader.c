#include "common-defines.h"
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "core/uart.h"
#include "core/system.h"
#include "core/firmware-info.h"
#include "core/crc.h"
#include "core/simple-timer.h"
#include "comms.h"
#include "bl-flash.h"

#define UART_PORT (GPIOA)
#define RX_PIN (GPIO3)
#define TX_PIN (GPIO2)

static void gpio_setup(void) {
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, RX_PIN|TX_PIN);
	gpio_set_af(UART_PORT, GPIO_AF7, RX_PIN|TX_PIN);
}
static void gpio_teardown(void) {
	gpio_mode_setup(UART_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, RX_PIN|TX_PIN);
	rcc_periph_clock_disable(RCC_GPIOA);
}

static bool is_device_id_packet(const comms_packet_t* pkt) {
	if (pkt->length != 2) return false;
	if (pkt->data[0] != BL_PACKET_DEVICE_ID_RES_DATA0) return false;
	for (uint8_t i=2;i<PACKET_DATA_LENGTH;i++) {
		if (pkt->data[i] != 0xff) return false;
	}
	return true;
}

static bool is_firware_length_packet(const comms_packet_t* pkt) {
	if (pkt->length != 5) return false;
	if (pkt->data[0] != BL_PACKET_FW_UPDATE_RES_DATA0) return false;
	for (uint8_t i=5;i<PACKET_DATA_LENGTH;i++) {
		if (pkt->data[i] != 0xff) return false;
	}
	return true;
}

static void jump_to_main(void) {
	/*
	typedef void (*void_fn)(void);
	uint32_t* reset_vector_entry = (uint32_t*)MAIN_APP_START_ADDR + 1;
	uint32_t* reset_vector = (uint32_t*)(*reset_vector_entry);
	void_fn jump_fn = (void_fn)reset_vector;
	jump_fn();
	*/

	vector_table_t* main_vector_table = (vector_table_t*)MAIN_APP_START_ADDR;
	main_vector_table->reset();
}

static bool validate_firmware_image(void) {
	firmware_info_t* firmware_info = (firmware_info_t*)FWINFO_ADDRESS;

	if (firmware_info->sentinel != FWINFO_SENTINEL) {
		return false;
	}

	if (firmware_info->device_id != DEVICE_ID) {
		return false;
	}

	const uint8_t* start_address = (const uint8_t*)FWINFO_VALIDATE_FROM;
	const uint32_t computed_crc = crc32(start_address,FWINFO_VALIDATE_LENGTH(firmware_info->length));
	return computed_crc == firmware_info->crc32;
}

#define SYNC_SEQ_0 (0xc4)
#define SYNC_SEQ_1 (0x55)
#define SYNC_SEQ_2 (0x7e)
#define SYNC_SEQ_3 (0x10)

#define DEFAULT_TIMEOUT (5000) // milliseconds

typedef enum bl_state_t {
	BL_State_Sync,
	BL_State_WaitForUpdateReq,
	BL_State_DeviceIDReq,
	BL_State_DeviceIDRes,
	BL_State_FWLengthReq,
	BL_State_FWLengthRes,
	BL_State_EraseApplication,
	BL_State_ReceiveFirmware,
	BL_State_Done
} bl_state_t;

static bl_state_t state = BL_State_Sync;
static uint32_t firmware_length = 0;
static uint32_t bytes_written = 0;
static uint8_t sync_seq[4] = {0};

static simple_timer_t timer;
static comms_packet_t pkt;
static void bootloading_fail(void) {
	comms_create_single_byte_packet(&pkt, BL_PACKET_NACK_DATA0);
	comms_write(&pkt);
	state = BL_State_Done;
}

static void check_for_timeout(void) {
	if (simple_timer_has_elapsed(&timer)) {
		bootloading_fail();
	}
}

int main(void) {
	system_setup();

	simple_timer_setup(&timer, DEFAULT_TIMEOUT, false);
	gpio_setup();
	uart_setup();
	comms_setup();


	while (state != BL_State_Done) {
		if (state == BL_State_Sync) {
			if (uart_data_available()) {
				sync_seq[0] = sync_seq[1];
				sync_seq[1] = sync_seq[2];
				sync_seq[2] = sync_seq[3];
				sync_seq[3] = uart_read_byte();

				bool is_match = sync_seq[0] == SYNC_SEQ_0;
				is_match = is_match && (sync_seq[1] == SYNC_SEQ_1);
				is_match = is_match && (sync_seq[2] == SYNC_SEQ_2);
				is_match = is_match && (sync_seq[3] == SYNC_SEQ_3);

				if (is_match) {
					comms_create_single_byte_packet(&pkt, BL_PACKET_SYNC_OBSERVED_DATA0);
					comms_write(&pkt);
					simple_timer_reset(&timer);
					state = BL_State_WaitForUpdateReq;
				} else {
					check_for_timeout();
				}
			} else {
				check_for_timeout();
			}

			continue;
		}

		comms_update();
		switch (state) {
			case BL_State_WaitForUpdateReq: {
				if (comms_packets_available()){
					comms_read(&pkt);
					if (comms_is_single_byte_packet(&pkt, BL_PACKET_FW_UPDATE_REQ_DATA0)) {
						comms_create_single_byte_packet(&pkt, BL_PACKET_FW_UPDATE_RES_DATA0);
						comms_write(&pkt);
						simple_timer_reset(&timer);
						state = BL_State_DeviceIDReq;
					} else {
						bootloading_fail();
					}
				} else {
					check_for_timeout();
				}
			} break;
			case BL_State_DeviceIDReq: {
				comms_create_single_byte_packet(&pkt, BL_PACKET_DEVICE_ID_REQ_DATA0);
				comms_write(&pkt);
				simple_timer_reset(&timer);
				state = BL_State_DeviceIDRes;
			} break;
			case BL_State_DeviceIDRes: {
				if (comms_packets_available()){
					comms_read(&pkt);
					if (is_device_id_packet(&pkt) && pkt.data[1] == DEVICE_ID) {
						simple_timer_reset(&timer);
						state = BL_State_FWLengthReq;
					} else {
						bootloading_fail();
					}
				} else {
					check_for_timeout();
				}
			} break;
			case BL_State_FWLengthReq: {
				simple_timer_reset(&timer);
				comms_create_single_byte_packet(&pkt, BL_PACKET_FW_LENGTH_REQ_DATA0);
				comms_write(&pkt);
				state = BL_State_FWLengthRes;
			} break;
			case BL_State_FWLengthRes: {
				if (comms_packets_available()){
					comms_read(&pkt);
					firmware_length = *(uint32_t*)(pkt.data+1);
					if (is_firware_length_packet(&pkt) && firmware_length <= MAX_FW_LENGTH) {
						simple_timer_reset(&timer);
						state = BL_State_EraseApplication;
					} else {
						bootloading_fail();
					}
				} else {
					check_for_timeout();
				}
			} break;
			case BL_State_EraseApplication: {
				bl_flash_erase_main_application();

				comms_create_single_byte_packet(&pkt, BL_PACKET_READY_FOR_DATA_DATA0);
				comms_write(&pkt);
				simple_timer_reset(&timer);
				state = BL_State_ReceiveFirmware;
			} break;
			case BL_State_ReceiveFirmware: {
				if (comms_packets_available()){
					comms_read(&pkt);

					const uint8_t packet_length = (pkt.length&0xf)+1;
					bl_flash_write(MAIN_APP_START_ADDR+bytes_written,pkt.data,packet_length);
					bytes_written += packet_length;
					simple_timer_reset(&timer);

					if (bytes_written >= firmware_length) {
						comms_create_single_byte_packet(&pkt, BL_PACKET_UPDATE_SUCCESSFUL_DATA0);
						comms_write(&pkt);
						state = BL_State_Done;
					} else {
						comms_create_single_byte_packet(&pkt, BL_PACKET_READY_FOR_DATA_DATA0);
						comms_write(&pkt);
					}
				} else {
					check_for_timeout();
				}
			} break;
			default:
				state = BL_State_Sync;
				break;
		}
	}

	system_delay(150); // wait for update successful packet transmission
	uart_teardown();
	gpio_teardown();
	system_teardown();
	
	if (validate_firmware_image()) {
		jump_to_main();
	} else {
		scb_reset_core();
	}
	return 0;
}
