/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "task.h"
#define	FILE_FD	0x8000
#define	FD_MASK	0x0fff
extern	MemInfo	 meminfo;
Task* get_taskptr(int32_t);

STRING get_fullpath(int32_t pid, STRING name) {
	Task	*task;
	STRING	cp, buffer;
	int32_t	size;

	size = strlen(name + 1);
	if(name[0] != '/') {
		task = get_taskptr(pid);
		if(task == 0) return 0;
		cp = task->pwd;
		size += strlen(cp);
	}
	buffer = alloc_mem(meminfo, pid, size);
	if(buffer == 0) return 0;
	buffer[0] = 0;
	if(name[0] != '/') strcpy(buffer, cp);
	strcat(buffer, name);
	return buffer;
}

int32_t open_file(int32_t pid, STRING name, int32_t opt) {
	STRING	path;
	int32_t	ret;

	ret = open_device(pid, name, opt);
	if(ret == -1) {
		path = get_fullpath(pid, name);
		if(path == 0) return -1;
		ret = minifs_open(pid, path, opt) | FILE_FD;
		free_mem(meminfo, pid, path);
	}
	return ret;
}

int32_t close_file(int32_t pid, int32_t fd) {
	int32_t	ret;

	if(fd & FILE_FD) {
		ret = minifs_close(pid, fd & FD_MASK);
	} else {
		ret = close_device(pid, fd);
	}
	return ret;
}

int32_t write_file(int32_t fd, STRING data, int32_t size) {
	int32_t	ret;

	if(fd & FILE_FD) {
		ret = minifs_write(fd & FD_MASK, data, size);
	} else {
		ret = write_device(fd, data, size);
	}
	return ret;
}

int32_t read_file(int32_t fd, STRING data, int32_t size) {
	int32_t	ret;

	if(fd & FILE_FD) {
		ret = minifs_read(fd & FD_MASK, data, size);
	} else {
		ret = read_device(fd, data, size);
	}
	return ret;
}

int32_t seek_file(int32_t fd, int32_t pos) {
	int32_t	ret;

	if(fd & FILE_FD) {
		ret = minifs_seek(fd & FD_MASK, pos);
	} else {
		ret = seek_device(fd, pos);
	}
	return ret;	
}

int32_t ioctl_file(int32_t fd, int32_t data, int32_t op){
	int32_t	ret;

	if(fd & FILE_FD) {
		ret = minifs_ioctl(fd & FD_MASK, data, op);
	} else {
		ret = ioctl_device(fd, data, op);
	}
	return ret;
}

int32_t delete_file(int32_t pid, STRING name) {
	STRING	path;
	int32_t	ret;

	path = get_fullpath(pid, name);
	if(path == 0) return -1;
	ret = minifs_delete(path);
	free_mem(meminfo, pid, path);
	return ret;
}

int32_t format(STRING device) {
	int32_t ret;

	ret = minifs_format(device);
	return ret;
}

int32_t get_dirent(int32_t pid, STRING pathname, int32_t index, STRING name, int32_t size) {
	STRING	path;
	int32_t	ret;

	path = get_fullpath(pid, pathname);
	if(path == 0) return -1;
	ret = minifs_dirent(path, index, name, size);
	free_mem(meminfo, pid, path);
	return ret;
}
