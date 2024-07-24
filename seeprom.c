/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "const.h"
#include "pubtype.h"
#include "seeprom.h"

#define SEEP_NUM	4
static SeepInfo se[SEEP_NUM];

#define WRITE	0xa0
#define READ	0xa1

void init_se() {
	int32_t	i;

	for(i = 0;i < SEEP_NUM;i++) se[i].id = 0;
}

int32_t open_se(uint16_t id, int32_t num, int32_t _fd, int32_t option) {
	SysInfo2 config;
	int32_t	 i, ret, fd;
	Byte	 name[6];

	if(num >= SEEP_NUM || id == 0) return -1;
	if(se[num].id != 0) return -1;
	strcpy(name, "se0");
	name[2] = '0' + num;
	ret = -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, name, 3) == 0) {
			se[num].size = config.address;
			se[num].page = config.size;
			ret = 0;
			break;
		}
	}
	if(ret == -1) return -1;
	strcpy(name, "i2c0");
	name[3] += config.bus;
	fd = open_device(id, name, 0);
	if(fd == -1) return -1;
	ioctl_device(fd, 2, I2C_ADDRSIZE);
	ioctl_device(fd, 1000, I2C_TIMEOVER);
	se[num].id = id;
	se[num].fd = fd;
	se[num].addr = 0;
	return 0;
}

int32_t close_se(uint16_t id, int32_t num) {
	if(num >= SEEP_NUM || id != se[num].id) return -1;
	se[num].id = 0;
	close_device(id, se[num].fd);
	return 0;	
}

int32_t write_se(int32_t num, STRING src, int32_t size) {
	int32_t	ret;

	if(num >= SEEP_NUM || se[num].id == 0) return -1;
	if(size == 0) return 0;
	ret = write_device(se[num].fd, src, size);
	return ret;
}

int32_t read_se(int32_t num, STRING dest, int32_t size) {
	int32_t	ret;

	if(num >= SEEP_NUM || se[num].id == 0) return -1;
	if(size == 0) return 0;
	ret = ioctl_device(se[num].fd, READ | ((se[num].addr >> 15) & 0x06), I2C_COMMAND);
	if(ret == -1) return -1;
	ret = read_device(se[num].fd, dest, size);
	return ret;
}

int32_t seek_se(int32_t num, int32_t addr) {
	int32_t	ret;

	if(num >= SEEP_NUM || se[num].id == 0) return -1;
	se[num].addr = addr;
	ret = ioctl_device(se[num].fd, WRITE | ((addr >> 15) & 0x06), I2C_COMMAND);
	if(ret == -1) return -1;
	seek_device(se[num].fd, addr);
	return addr;
}

int32_t ioctl_se(int32_t num, int32_t data, int32_t op) {
	int32_t ret;

	if(num >= SEEP_NUM || se[num].id == 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		ret = FILE_DEVICE | FLUSH_DEVICE;
		break;
	case DISK_SIZE:
		ret = se[num].size;
		break;
	case DISK_PAGE:
		ret = se[num].page;
		break;
	default:
		ret = -1;
	}
	return ret;
}
