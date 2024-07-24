/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"

int32_t get_config(int32_t ent, SysInfo *config) {
	STRING	source;

	if(ent * sizeof(SysInfo) >= 0x100) return 0;
	source = (STRING)(ent * sizeof(SysInfo) + 0x400);
	if(*source == 0xff) return 0;
	memcpy((STRING)config, source, sizeof(SysInfo));
	return 1;
}
