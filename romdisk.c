/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "const.h"
#include "romdisk.h"

#if defined(SH_7045)
 #define FLUSH_START	0x30000
 #define FLUSH_END	0x80000
#endif

static int32_t		flush_id;
static STRING		position;
int32_t set_flush_freq(int32_t);
int32_t load_program(void);
int32_t write_program(STRING, STRING, int32_t);
int32_t erase_program(STRING);

void init_rom() {
	uint32_t	 i, f;
	SysInfo		 config;

	f = 25;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, "sci", 3) == 0) {
			f = config.irq;
			break;
		}
	}
	flush_id = 0;
	set_flush_freq(f);
	position = (STRING)FLUSH_START;
}

int32_t write_rom(int32_t num, STRING data, int32_t size){
	if(num != 0) return -1;
	if((int32_t)position >= (FLUSH_END - FLUSH_START + size)) return -1;
	return write_program(position + FLUSH_START, data, size);
}

int32_t read_rom(int32_t num, STRING data, int32_t size) {
	int32_t	i;

	if(num != 0) return -1;
	if((int32_t)position >= (FLUSH_END - FLUSH_START + size)) return -1;
	memcpy(data, position + FLUSH_START, size);
	return size;
}

int32_t open_rom(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	if(num != 0 || id == 0) return -1;
	if(flush_id != 0) return -1;
	flush_id = id;
	return 0;
}

int32_t close_rom(uint16_t id, int32_t num) {
	if(num != 0 || id != flush_id) return -1;
	if(flush_id != id && flush_id != 1) return -1;
	flush_id = 0;
	return 0;
}

int32_t seek_rom(int32_t num, int32_t pos) {
	if(num != 0) return -1;
	if(pos < 0 || (pos >= (FLUSH_END - FLUSH_START))) return -1; // AT 2004/08/27 
	position = (STRING)pos;
	return pos;
}

int32_t ioctl_rom(int32_t num, int32_t data, int32_t op) {
	int32_t	ret;

	if(num != 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		ret = FILE_DEVICE | FLUSH_DEVICE | RO_DEVICE;
		break;
	case DISK_SIZE:
		ret = FLUSH_END - FLUSH_START;
		break;
	case DISK_PAGE:
		ret = 128;
		break;
	case FLUSH_PAGESIZE:
		ret = 128;
		break;
	case FLUSH_ERASE:
		ret = erase_program((STRING)data);
		break;
	case FLUSH_FREQ:
		set_flush_freq(data);
		break;
	default:
		ret = -1;
	}
	return ret;
}
