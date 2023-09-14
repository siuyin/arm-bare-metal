#include "core/simple-timer.h"
#include "core/system.h"

void simple_timer_setup(simple_timer_t* tm, uint64_t wait_time, bool auto_reset) {
	tm->wait_time = wait_time;
	tm->target_time = system_get_ticks() + wait_time;
	tm->auto_reset = auto_reset;
	tm->has_elapsed = false;
}

bool simple_timer_has_elapsed(simple_timer_t* tm) {
	uint64_t now = system_get_ticks();
	bool has_elapsed = now >= tm->target_time;
	
	if (tm->has_elapsed) return false;

	if (has_elapsed) {
	       if (tm->auto_reset) {
			uint64_t drift = now - tm->target_time;
			tm->target_time = (now + tm->wait_time) - drift;
		} else {
			tm->has_elapsed = true;
		}
	}
	return has_elapsed;
}
void simple_timer_reset(simple_timer_t* tm) {
	simple_timer_setup(tm, tm->wait_time, tm->auto_reset);
}
