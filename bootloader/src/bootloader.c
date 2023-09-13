#include "common-defines.h"
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "core/uart.h"
#include "core/system.h"
#include "comms.h"

#define BOOTLOADER_SIZE (0X8000U)
#define MAIN_APP_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

#define UART_PORT (GPIOA)
#define RX_PIN (GPIO3)
#define TX_PIN (GPIO2)

static void gpio_setup(void) {
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, RX_PIN|TX_PIN);
	gpio_set_af(UART_PORT, GPIO_AF7, RX_PIN|TX_PIN);
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

int main(void) {
	system_setup();
	gpio_setup();
	uart_setup();
	comms_setup();

	comms_packet_t pkt = {
		.length = 9,
		.data = {1,2,3,4,5,6,7,8,9,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
		.crc = 0
	};
	pkt.crc = comms_compute_crc(&pkt);

	while (true) {
		comms_update();

		comms_packet_t rx_pkt;
		if (comms_packets_available()) {
			comms_read(&rx_pkt);
		}

		comms_write(&pkt);
		system_delay(500);
	}

	// TODO: teardown, de-initialized peripherals.

	jump_to_main();
	return 0;
}
