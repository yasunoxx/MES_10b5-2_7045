/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "regdef.h"

void set_bussize(int cs, int bus) {
	unsigned short mask;

	mask = 1 << cs;
	if(bus == 8) {
		BCR1 &= ~mask;
	} else if(bus == 16) {
		BCR1 |= mask;
	}
}

void set_wait(int cs) {
	unsigned short mask;

	mask = 0xf << (cs * 4);
//	WCR |= mask;
	WCR1 |= mask;
	// FIXME: configure WCR2?(if use DMA mode) -- yasunoxx 20240703
}
