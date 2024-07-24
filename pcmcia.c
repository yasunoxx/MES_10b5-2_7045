/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "const.h"

#define CIS_TPL_VERS_1		0x15
#define CIS_TPL_NO_LINK		0x14
#define CIS_TPL_END		0xff
#define CIS_TPL_CONFIG		0x1a
#define CIS_TPL_CFTABLE_ENTRY	0x1b
#define CIS_TPL_MANFID		0x20
#define CIS_TPL_FUNCE		0x22
#define LAN_NID			4

unsigned int pccard_init(unsigned int address) {
	volatile int w;
	int	i, j, index, code, size, n, ccr, ret;
	char	*CIS, *str, cardinfo[128];

	CIS = (char*)address;
	CIS[0x1000] = 0;
	for(w = 0;w < 200000;w++);
	CIS[0x800] = 0;
	for(w = 0;w < 100000;w++);
	index = code = 0;
	ret = -1;
	while(code != CIS_TPL_NO_LINK && code != CIS_TPL_END) {
		code = CIS[index * 2];
		size = CIS[(index + 1)* 2];
		switch(code) {
		case CIS_TPL_VERS_1:
			if(code == CIS_TPL_VERS_1) {
				for(j = 0;j < size;j++) {
					cardinfo[j] = CIS[(index + 2 + j) * 2];
				}
			}
			break;
		case CIS_TPL_CONFIG:
			ccr = ((int)(CIS[(index + 5) * 2]) & 0xff) << 8;
			ccr |= (int)(CIS[(index + 4) * 2]) & 0xff;
			ret = 0;
			break;
		}
		index += size + 2;
	}
	CIS[ccr] = 1;
	return ret;
}
