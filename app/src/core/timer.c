#include "core/timer.h"
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>


#define AUTO_RELOAD_VALUE (1000)
void timer_setup(void) {
	rcc_periph_clock_enable(RCC_TIM2);
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_PWM1);
	timer_enable_counter(TIM2);
	timer_enable_oc_output(TIM2, TIM_OC1);
	timer_set_prescaler(TIM2, 84-1); // CPU_FREQ=84MHz => timer tick = 1us
	timer_set_period(TIM2, AUTO_RELOAD_VALUE-1); // period is 1000 ticks = 1ms
}

// timer_pwm_set_duty_cycle percentage. eg. 66.7
void timer_pwm_set_duty_cycle(float duty_cycle) {
	timer_set_oc_value(TIM2, TIM_OC1, (uint32_t)(duty_cycle/100.0f * AUTO_RELOAD_VALUE));
}
