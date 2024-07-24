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
#include "devdef.h"

uint32_t get_fd(STRING name) {
	uint32_t	n, major, minor;
	Byte		c, buffer[8];

	memcpy(buffer, name, 8);
	buffer[7] = 0;
	n = strlen(buffer);
	if(n < 2) return 0;
	c = buffer[n - 1];
	if(!isdigit(c)) return 0;
	buffer[n - 1] = 0;
	minor = c - '0';
	for(n = 0;n < DEVNUM;n++) {
		if(strcmp(DevDef[n].name, buffer) == 0) {
			major =  DevDef[n].major;
			return (major << 4) + minor;
		}
	}
	return 0;
}

static uint32_t check_fd(Byte fd) {
	uint32_t	n, major;

	major = (fd >> 4) & 0x0f;
	for(n = 0;n < DEVNUM;n++) if(major == DevDef[n].major) return n;
	return -1;
}

void init_device() {
	int32_t	n;

	for(n = 0;n < DEVNUM;n++) (*(DevDef[n].init_func))();
}

int32_t open_device(uint16_t id, STRING name, uint32_t option){
	uint32_t	n, fd, major, minor, ccrsv;

	if((fd = get_fd(name)) == 0) return -1;
	n = check_fd(fd);
	major = (fd >> 4) & 0x0f;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	if((*(DevDef[n].open_func))(id, minor, fd, option) != 0) fd = -1;
	set_flag(ccrsv);
	return fd;
}

int32_t close_device(uint16_t id, uint32_t fd){
	uint32_t	n, ret, minor, ccrsv;

	if((n = check_fd(fd)) == -1) return -1;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	ret = (*(DevDef[n].close_func))(id, minor);
	set_flag(ccrsv);
	return ret;
}

int32_t write_device(uint32_t fd, STRING data, uint32_t size){
	uint32_t	n, ret, minor, ccrsv;

	if((n = check_fd(fd)) == -1) return -1;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	ret = (*(DevDef[n].write_func))(minor, data, size);
	set_flag(ccrsv);
	return ret;
}

int32_t read_device(uint32_t fd, STRING data, uint32_t size){
	uint32_t	n, ret, minor, ccrsv;

	if((n = check_fd(fd)) == -1) return -1;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	ret = (*(DevDef[n].read_func))(minor, data, size);
	set_flag(ccrsv);
	return ret;
}

int32_t seek_device(uint32_t fd, uint32_t position){
	uint32_t	n, ret, minor, ccrsv;

	if((n = check_fd(fd)) == -1) return -1;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	ret = (*(DevDef[n].seek_func))(minor, position);
	set_flag(ccrsv);
	return ret;
}

int32_t ioctl_device(uint32_t fd, uint32_t data, uint32_t op){
	uint32_t	n, ret, minor, ccrsv;

	if((n = check_fd(fd)) == -1) return -1;
	minor = fd & 0x0f;
	ccrsv = int_disable();
	ret = (*(DevDef[n].ioctl_func))(minor, data, op);
	set_flag(ccrsv);
	return ret;
}
