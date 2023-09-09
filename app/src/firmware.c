#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "core/system.h"

#define LED_PORT (GPIOD)
#define LED_PIN (GPIO2)

static void gpio_setup(void) {
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

int main(void) {
	system_setup();
	gpio_setup();

	uint64_t start_time = system_get_ticks();
	while (1) {
		if (system_get_ticks() - start_time >= 1000) {
			gpio_toggle(LED_PORT, LED_PIN);
			start_time = system_get_ticks();
		}
	}

	return 0;
}
