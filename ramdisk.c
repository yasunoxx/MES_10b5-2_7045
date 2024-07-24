/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "const.h"
#include "ramdisk.h"

typedef struct {
	int32_t	id;
	int32_t	offset;
	int32_t	page;
	int32_t	size;
	int32_t	current;
} RAMInfo;
#define RAM_NUM	1	
static RAMInfo	ram[RAM_NUM];

void init_ram() {
	int32_t	i;

	for(i = 0;i < RAM_NUM;i++) ram[i].id = 0;
}

int32_t open_ram(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	SysInfo2 config;
	int32_t	 i, ret;
	Byte	 name[6];

	if(num >= RAM_NUM) return -1;
	if(ram[num].id != 0) return -1;
	strcpy(name, "ram0");
	name[3] = '0' + num;
	ret = -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, name, 5) == 0) {
			ret = 0;
			ram[num].offset = config.address;
			ram[num].size = config.size;
			ram[num].page = 1 << config.bus;
			ram[num].current = 0;
			break;
		}
	}
	if(ret == -1) return -1;
	ram[num].id = id;
	return 0;
}

int32_t close_ram(uint16_t id, int32_t num) {
	if(num >= RAM_NUM) return -1;
	if(ram[num].id != id) return -1;
	ram[num].id = 0;
	return 0;
}

int32_t write_ram(int32_t num, STRING data, int32_t size){
	if(num >= RAM_NUM) return -1;
	if(ram[num].current + size >= ram[num].size) return -1; 
	memcpy((STRING)(ram[num].offset + ram[num].current), data, size);
	return size;
}

int32_t read_ram(int32_t num, STRING data, int32_t size) {
	if(num >= RAM_NUM) return -1;
	if(ram[num].current + size >= ram[num].size) return -1;
	memcpy(data, (STRING)(ram[num].offset + ram[num].current), size);
	return size;
}


int32_t seek_ram(int32_t num, int32_t pos) {
	if(num >= RAM_NUM) return -1;
	if(pos >= ram[num].size) return -1;
	ram[num].current = pos;
	return pos;
}

int32_t ioctl_ram(int32_t num, int32_t data, int32_t op) {
	int32_t	ret;

	if(num != 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		ret = FILE_DEVICE;
		break;
	case DISK_SIZE:
		ret = ram[num].size;
		break;
	case DISK_PAGE:
		ret = ram[num].page;
		break;
	default:
		ret = -1;
	}
	return ret;
}
