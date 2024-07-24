/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"
#include "const.h"
#include "regdef.h"

extern uint32_t user1_start, user2_start;
static uint32_t mem_addr, mem_size;

void get_meminfo(uint32_t *addr, uint32_t *size) {
	*addr = mem_addr;
	*size = mem_size;
}

int main() {
	void		 (*func)();
	volatile int32_t i;
	SysInfo2	 config;
	volatile uint16_t	*wreg;
	volatile uint8_t	*breg;

	mem_addr = mem_size = 0;
	for(i = 0;get_config(i, &config);i++) {
		if(strcmp(config.name, "sram") == 0 || strcmp(config.name, "dram") == 0 ) {
			set_bussize(config.cs, config.bus);
			mem_addr = config.address;
			mem_size = config.size;
			if(strcmp(config.name, "dram") == 0 ) {
				DCR = 0x0021;	/* 8bit-bus 10bit-address */
				RTCSR = 0x001a;	/* clk x 2048 */
				RTCOR = 10;	/* 1.5[ms] reflesh cycle */
			}
		} else if(strcmp(config.name, "io") == 0) {
			if(config.bus == 16) {
				wreg = (volatile uint16_t *)config.address;
				*wreg = config.size;
			} else {
				breg = (volatile uint8_t *)config.address;
				*breg = config.size;
			}
		}
	}
	init_device();
	func = (FLMCR1 & 0x80) ? (void*)(&user1_start) : (void*)(&user2_start);
	(*func)();
}
