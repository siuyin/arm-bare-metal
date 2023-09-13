#include "common-defines.h"
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "core/uart.h"
#include "core/system.h"
#include "comms.h"
#include "bl-flash.h"

#define BOOTLOADER_SIZE (0X8000U)
#define MAIN_APP_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

#define UART_PORT (GPIOA)
#define RX_PIN (GPIO3)
#define TX_PIN (GPIO2)

//static void gpio_setup(void) {
//	rcc_periph_clock_enable(RCC_GPIOA);
//
//	gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, RX_PIN|TX_PIN);
//	gpio_set_af(UART_PORT, GPIO_AF7, RX_PIN|TX_PIN);
//}


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
	// gpio_setup();
	// uart_setup();
	// comms_setup();

	uint8_t data[1024] = {0};
	for (uint16_t i=0;i<1024;i++) {
		data[i]=i&0xff;
	}

	bl_flash_erase_main_application();
	bl_flash_write(0x08008000, data, 1024);
	bl_flash_write(0x0800c000, data, 1024);
	bl_flash_write(0x08010000, data, 1024);
	bl_flash_write(0x08020000, data, 1024);
	bl_flash_write(0x08040000, data, 1024);
	bl_flash_write(0x08060000, data, 1024);

	while (true) {
	}

	// TODO: teardown, de-initialized peripherals.

	jump_to_main();
	return 0;
}
