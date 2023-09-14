#include "common-defines.h"
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "core/uart.h"
#include "core/system.h"
#include "core/simple-timer.h"
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

	simple_timer_t tm,tm2;
	simple_timer_setup(&tm,1000,false);
	simple_timer_setup(&tm2,2000,true);
	// gpio_setup();
	// uart_setup();
	// comms_setup();

	while (true) {
		if (simple_timer_has_elapsed(&tm)) {
			volatile int x = 0;
			x++; // log message here in debugger
		}
		if (simple_timer_has_elapsed(&tm2)) {
			simple_timer_reset(&tm);
		}
	}

	// TODO: teardown, de-initialized peripherals.

	jump_to_main();
	return 0;
}
