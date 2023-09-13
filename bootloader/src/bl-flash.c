#include <libopencm3/stm32/flash.h>
#include "bl-flash.h"

#define MAIN_APP_SECTOR_START (2)
#define MAIN_APP_SECTOR_END (7)

void bl_flash_erase_main_application(void) {
	flash_unlock();
	for (uint8_t i=MAIN_APP_SECTOR_START; i<=MAIN_APP_SECTOR_END; i++) {
		flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
	}
	flash_lock();
}

void bl_flash_write(const uint32_t addr, const uint8_t* dat, const uint32_t len) {
	flash_unlock();
	flash_program(addr,dat,len);
	flash_lock();
}
