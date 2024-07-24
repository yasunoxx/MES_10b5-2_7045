/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "minifs.h"

#define SuperPID 1
#define FD_NUM	 6
#define DISK_NUM 4

extern MemInfo	 meminfo;
static DiskInfo disk[DISK_NUM];
static FdInfo filefd[FD_NUM];

static int32_t get_disknum(STRING path) {
	int32_t	i;
	Byte	name[8];
	STRING	ptr;

	strncpy(name, path, 7);
	ptr = strchr(name, '/');
	if(ptr != 0) *ptr = 0;
	for(i = 0;i < DISK_NUM;i++) {
		if(disk[i].fd == 0) continue;
		if(strcmp(name, disk[i].name) == 0) return i;
	}
	return -1;
}

static void blank_page(int32_t num, int32_t page) {
	Byte	buffer[disk[num].pagesize];

	memset(buffer, 0xff, disk[num].pagesize);
	seek_device(disk[num].fd, page * disk[num].pagesize);
	write_device(disk[num].fd, buffer, disk[num].pagesize);
}

static uint16_t *load_fat(int32_t num) {
	int32_t	i;
	STRING	ptr, fatptr;

	fatptr = alloc_mem(meminfo, SuperPID, disk[num].firstfat * disk[num].pagesize);
	for(i = 0;i < disk[num].firstfat;i++) {
		ptr = &fatptr[i * disk[num].pagesize];
		seek_device(disk[num].fd, i * disk[num].pagesize);
		read_device(disk[num].fd, ptr, disk[num].pagesize);
	}
	return (uint16_t*)fatptr;
}

static void save_fat(int32_t num, uint16_t *fatptr) {
	int32_t	i;
	STRING	ptr;

	ptr = (STRING)fatptr;
	for(i = 0;i < disk[num].firstfat;i++) {
		seek_device(disk[num].fd, i * disk[num].pagesize);
		write_device(disk[num].fd, &ptr[i * disk[num].pagesize], disk[num].pagesize);
	}
}

static STRING load_data(int32_t num, short *fats, short fat, int32_t offset, int32_t maxsize, int32_t *getsize) {
	int32_t	 count, current;
	uint16_t node;
	STRING	 data;

	data = alloc_mem(meminfo, SuperPID, maxsize);
	if(data == 0) return 0;
	bzero(data, maxsize);
	for(node = fat, current = 0;node != TAIL;node = fats[node],current += disk[num].pagesize ) {
		if(current >= (offset + maxsize)) break;
		if(current >= offset) {
			seek_device(disk[num].fd, node * disk[num].pagesize);
			read_device(disk[num].fd, &data[current - offset], disk[num].pagesize);
		}
	}
	*getsize = current - offset;
	return data;
}

static void save_data(int32_t num, short *fats, short fat, int32_t offset, int32_t size, STRING data) {
	int32_t	 count, current;
	uint16_t node;

	for(node = fat, current = 0;node != TAIL;node = fats[node],current += disk[num].pagesize ) {
		if(current >= (offset + size)) break;
		if(current >= offset) {
			seek_device(disk[num].fd, node * disk[num].pagesize);
			write_device(disk[num].fd, &data[current - offset], disk[num].pagesize);
		}
	}
	free_mem(meminfo, SuperPID, data);
}

int32_t minifs_format(STRING path) {
	int32_t	 num, i, n, secnum;
	uint16_t *fats;

	num = get_disknum(path);
	if(num == -1) return -1;
	if(disk[num].info & RO_DEVICE) return -1;
	secnum = disk[num].disksize / disk[num].pagesize;
	n = (secnum * FATSIZEOF) / disk[num].pagesize;
	fats = load_fat(num);
	for(i = 0;i < secnum;i++) fats[i] = FREE;
	fats[0] = n;
	fats[n] = TAIL;
	save_fat(num, fats);
	free_mem(meminfo, SuperPID, (STRING)fats);
	blank_page(num, n);
	return 0;
}

static STRING strpathcmp(STRING path, STRING name) {
	int32_t	i;

	if(path[0] != '/') return 0;
	for(i = 0;path[i + 1] != '/' && path[i + 1] != 0;i++) {
		if(name[i] != path[i + 1]) return 0;
	}
	return &(path[i + 1]);
}

static uint16_t get_node(int32_t num, STRING dirname) {
	int32_t	 i, ret, entnum, datasize;
	uint16_t *fats, node;
	STRING	 path, ptr;
	Entry	 *entries;

	path = dirname;
	fats = load_fat(num);
	node = fats[0];
	while(path[0] != 0) {
		entries = (Entry*)load_data(num, fats, node, 0, CACHESIZ, &datasize);
		entnum = datasize / sizeof(Entry);
		for(i = 0;i < entnum;i++) {
			if(entries[i].name[0] == 0xff || entries[i].name[0] == 0) continue;
			if(!(entries[i].attr & ATTR_DIR)) continue;
			ptr = strpathcmp(path, entries[i].name);
			if(ptr != 0) break;
		}
		node = entries[i].fat;
		free_mem(meminfo, SuperPID, (STRING)entries);
		if(ptr == 0) {
			free_mem(meminfo, SuperPID, (STRING)fats);
			return 0;
		}
		path = ptr;
	}
	free_mem(meminfo, SuperPID, (STRING)fats);
	return node;
}

static uint16_t free_fat(int32_t num, uint16_t *fats) {
	int32_t	i, secnum;

	secnum = disk[num].disksize / disk[num].pagesize;
	for(i = disk[num].firstfat;i < secnum;i++) {
		if(fats[i] == FREE) return i;
	}
	return 0;
}

static int32_t flushfile(int32_t num, int32_t fd, short *fats){
	int32_t	 i, addsize, addpage;
	uint16_t node, newfat;
	Entry	 *ent;

	ent = &(filefd[fd].ent);
	addsize = ent->size - filefd[fd].offset - filefd[fd].areasize;
	if(addsize > 0) {
		addpage = (addsize - 1) / disk[num].pagesize + 1;
		for(node = ent->fat;fats[node] != TAIL;node = fats[node]);
		for(i = 0;i < addpage;i++) {
			newfat = free_fat(num, fats);
			if(newfat == 0) {
				free_mem(meminfo, SuperPID, filefd[fd].cache);
				filefd[fd].cache = 0;
				return -1;
			}
			fats[node] = newfat;
			node = fats[node];
			fats[node] = TAIL;
		}
		save_fat(num, fats);
		filefd[fd].areasize += addpage * disk[num].pagesize;
	}
	save_data(num, fats, ent->fat, filefd[fd].offset, filefd[fd].areasize, filefd[fd].cache);
	return 0;
}

int32_t minifs_dirent(STRING pathname, int32_t index, STRING name, int32_t size) {
	int32_t	 ret, i, count, entnum, datasize, num;
	STRING	 dirname;
	uint16_t *fats, root;
	Entry	 *entries;

	if(pathname[0] != '/') return -1;
	num = get_disknum(&pathname[1]);
	if(num == -1) return -1;
	dirname = strchr(&pathname[1], '/');
	if(dirname == 0) return -1;
	dirname++;
	root = get_node(num, dirname);
	if(root == 0) return -1;
	fats = load_fat(num);
	entries = (Entry*)load_data(num, fats, root, 0, CACHESIZ, &datasize);
	entnum = datasize / sizeof(Entry);
	count = 0;
	ret = -2;
	for(i = 0;i < entnum;i++) {
		if(entries[i].name[0] == 0xff || entries[i].name[0] == 0) continue;
		if(count++ == index) {
			strncpy(name, entries[i].name, size);
			ret = entries[i].size;
			break;
		}
	}
	free_mem(meminfo, SuperPID, (STRING)fats);
	free_mem(meminfo, SuperPID, (STRING)entries);
	return ret;
}

int32_t minifs_open(int32_t id, STRING pathname, int32_t option) {
	Byte	 dirname[MAXPATHSIZ];
	STRING	 filename, ptr;
	uint16_t *fats, root, newfat, addfat;
	int32_t	 num, ret, i, datasize, entnum, fd;
	Entry	 *entries;

	for(fd = 0;fd < FD_NUM;fd++) {
		if(filefd[fd].id == 0) break;
	}
	if(fd >= FD_NUM) return -1;
	if(pathname[0] != '/') return -1;
	num = get_disknum(&pathname[1]);
	if(num == -1) return -1;
	ptr = strchr(&pathname[1], '/');
	if(ptr == 0) return -1;
	strcpy(dirname, ptr);
	filename = strrchr(dirname, '/');
	if(filename == 0) return -1;
	*(filename++) = 0;
	root = get_node(num, dirname);
	if(root == 0) return -1;
	fats = load_fat(num);
	entries = (Entry*)load_data(num, fats, root, 0, CACHESIZ, &datasize);
	entnum = datasize / sizeof(Entry);
	for(i = 0;i < entnum;i++) {
		if(entries[i].name[0] == 0xff || entries[i].name[0] == 0) continue;
		if(strcmp(entries[i].name, filename) == 0) {
			memcpy((STRING)&(filefd[fd].ent), &(entries[i]), sizeof(Entry));
			free_mem(meminfo, SuperPID, (STRING)entries);
			break;
		}
	}
	if(i == entnum) {
		if(option & READ_FILE || disk[num].info & RO_DEVICE) {
			free_mem(meminfo, SuperPID, (STRING)fats);
			free_mem(meminfo, SuperPID, (STRING)entries);
			return -1;
		}
		newfat = free_fat(num, fats);
		fats[newfat] = TAIL;
		if(newfat == 0) {
			free_mem(meminfo, SuperPID, (STRING)fats);
			free_mem(meminfo, SuperPID, (STRING)entries);
			return -1;
		}
		ret = -1;
		for(i = 0;i < entnum;i++) {
			if(entries[i].name[0] == 0xff) {
				ret = 0;
				break;
			}
		}
		if(ret == -1) {
			addfat = free_fat(num, fats);
			if(addfat != 0) {
				ret = 0;
				fats[root] = addfat;
				fats[addfat] = TAIL;
				blank_page(num, addfat);
				entries = (Entry*)load_data(num, fats, root, 0, CACHESIZ, &datasize);
				entnum = datasize / sizeof(Entry);
				for(i = 0;i < entnum;i++) {
					if(entries[i].name[0] == 0xff) {
						ret = 0;
						break;
					}
				}
			}
		}
		if(ret == -1) {
			free_mem(meminfo, SuperPID, (STRING)fats);
			free_mem(meminfo, SuperPID, (STRING)entries);
			return -1;
		}
		strcpy(entries[i].name, filename);
		entries[i].size = 0;
		entries[i].fat = newfat;
		fats[newfat] = TAIL;
		save_fat(num, fats);
		entries[i].attr = 0;
		if(option & DIR_FILE) {
			blank_page(num, newfat);
			entries[i].attr |= ATTR_DIR;
		}
		memcpy((STRING)&(filefd[fd].ent), &(entries[i]), sizeof(Entry));
		save_data(num, fats, root, 0, datasize, (STRING )entries);
	}
	filefd[fd].entindex = i;
	filefd[fd].current = (option & APPEND_FILE) ? filefd[fd].ent.size : 0;
	filefd[fd].offset = (filefd[fd].current / CACHESIZ) * CACHESIZ;
	filefd[fd].option = option;
	filefd[fd].id = id;
	filefd[fd].root = root;
	filefd[fd].cache = load_data(num, fats, filefd[fd].ent.fat, filefd[fd].offset, CACHESIZ, &(filefd[fd].areasize));
	filefd[fd].num = num;
	free_mem(meminfo, SuperPID, (STRING)fats);
	return fd;
}

int32_t minifs_close(int32_t id, int32_t fd) {
	int32_t	 num, i, datasize;
	Entry	 *entries;
	uint16_t *fats;
	STRING	 buffer;

	if(fd >= FD_NUM) return -1;
	num = filefd[fd].num;
	if(filefd[fd].option & READ_FILE) {
		free_mem(meminfo, SuperPID, (STRING)filefd[fd].cache);
	} else {
		i = filefd[fd].entindex;
		fats = load_fat(num);
		entries = (Entry*)load_data(num, fats, filefd[fd].root, 0, CACHESIZ, &datasize);
		memcpy(&(entries[i]), (STRING)&(filefd[fd].ent), sizeof(Entry));
		save_data(num, fats, filefd[fd].root, 0, datasize, (STRING)entries);
		flushfile(num, fd, fats);
		free_mem(meminfo, SuperPID, (STRING)fats);
	}
	filefd[fd].id = 0;
	return 0;
}

int32_t minifs_write(int32_t fd, STRING data, int32_t size) {
	uint16_t *fats;
	int32_t	 num;
	int32_t	 remain, cache_remain, copysize;
	STRING	 buffer, src, dest;
	Entry	 *ent;

	if(fd >= FD_NUM) return -1;
	num = filefd[fd].num;
	if(filefd[fd].option & READ_FILE || disk[num].info & RO_DEVICE) return 0;
	ent = &(filefd[fd].ent);
	if(ent->attr & ATTR_DIR) return 0;
	buffer = filefd[fd].cache;
	remain = size;
	while(remain > 0) {
		cache_remain = CACHESIZ - filefd[fd].current + filefd[fd].offset;
		if(cache_remain <= 0) {
			fats = load_fat(num);
			flushfile(num, fd, fats);
			filefd[fd].offset = filefd[fd].current;
			filefd[fd].cache = load_data(num, fats, ent->fat, filefd[fd].offset, CACHESIZ, &(filefd[fd].areasize));
			buffer = filefd[fd].cache;
			cache_remain = CACHESIZ - filefd[fd].current + filefd[fd].offset;
			save_fat(num, fats);
			free_mem(meminfo, SuperPID, (STRING)fats);
		}
		copysize = (remain <= cache_remain) ? remain : cache_remain;
		dest = &buffer[filefd[fd].current - filefd[fd].offset];
		src = &data[size - remain];
		memcpy(dest, src, copysize);
		remain -= copysize;
		filefd[fd].current += copysize;
		if(ent->size < filefd[fd].current) ent->size = filefd[fd].current;
	}
	return size;
}

int32_t minifs_read(int32_t fd, STRING data, int32_t size) {
	uint16_t *fats;
	int32_t	 num, remain, cache_remain, copysize, a, b;
	STRING	 buffer, src, dest;
	Entry	 *ent;

	if(fd >= FD_NUM) return -1;
	num = filefd[fd].num;
	buffer = filefd[fd].cache;
	ent = &(filefd[fd].ent);
	copysize = remain = size;
	while(remain > 0 && copysize > 0) {
		cache_remain = CACHESIZ - filefd[fd].current + filefd[fd].offset;
		if(cache_remain <= 0) {
			fats = load_fat(num);
			free_mem(meminfo, SuperPID, (STRING)filefd[fd].cache);
			filefd[fd].offset = filefd[fd].current;
			filefd[fd].cache = load_data(num, fats, ent->fat, filefd[fd].offset, CACHESIZ, &(filefd[fd].areasize));
			buffer = filefd[fd].cache;
			cache_remain = CACHESIZ - filefd[fd].current + filefd[fd].offset;
			free_mem(meminfo, SuperPID, (STRING)fats);
		}
		copysize = (remain <= cache_remain) ? remain : cache_remain;
		if(copysize > (ent->size - filefd[fd].current)) {
			copysize = ent->size - filefd[fd].current;
		}
		src = &buffer[filefd[fd].current - filefd[fd].offset];
		dest = &data[size - remain];
		memcpy(dest, src, copysize);
		remain -= copysize;
		filefd[fd].current += copysize;
	}
	return size - remain;
}

int32_t minifs_seek(int32_t fd, int32_t _pos) {
	int32_t	 num, pos;
	Entry	 *ent;
	uint16_t *fats;

	if(fd >= FD_NUM) return -1;
	num = filefd[fd].num;
	pos = _pos;
	ent = &(filefd[fd].ent);
	if(pos > ent->size) pos = ent->size;
	filefd[fd].current = pos;
	if(!(pos < filefd[fd].offset && (pos - CACHESIZ) >= filefd[fd].offset)) {
		fats = load_fat(num);
		if(!(filefd[fd].option & READ_FILE) && !(disk[num].info & RO_DEVICE)) {
			flushfile(num, fd, fats);
		}
		free_mem(meminfo, SuperPID, (STRING)filefd[fd].cache);
		filefd[fd].offset = filefd[fd].current;
		filefd[fd].cache = load_data(num, fats, ent->fat, filefd[fd].offset, CACHESIZ, &(filefd[fd].areasize));
		free_mem(meminfo, SuperPID, (STRING)fats);
	}
	return pos;
}

static int32_t minifs_size(int32_t fd) {
	int32_t	 num, pos;
	Entry	 *ent;

	if(fd >= FD_NUM) return -1;
	if(filefd[fd].id == 0) return -1;
	return filefd[fd].ent.size;
}

int32_t minifs_ioctl(int32_t fd, int32_t data, int32_t op) {
	int32_t	ret;

	if(fd >= FD_NUM) return -1;
	switch(op) {
	case FILE_SIZE:
		ret = minifs_size(fd);
		break;
	default:
		ret = -1;
	}
	return ret;
}

static int32_t del_all(int32_t num, uint16_t root, uint16_t *fats) {
	uint16_t node, next;
	int32_t	 i, datasize, entnum;
	Entry	 *entries;

	entries = (Entry*)load_data(num, fats, root, 0, CACHESIZ, &datasize);
	entnum = datasize / sizeof(Entry);
	for(i = 0;i < entnum;i++) {
		if(entries[i].name[0] == 0xff || entries[i].name[0] == 0) continue;
		if(entries[i].attr & ATTR_DIR) {
			del_all(num, entries[i].fat, fats);
		}
		for(node = entries[i].fat;next != TAIL;node = next) {
			next = fats[node];
			fats[node] = FREE;
		}
	}
	memset((STRING)entries, 0xff, datasize);
	save_data(num, fats, root, 0, datasize, (STRING)entries);
	return 0;
}

int32_t minifs_delete(STRING pathname) {
	Byte	 dirname[MAXPATHSIZ];
	STRING	 ptr, filename;
	uint16_t *fats, root, node, next;
	int32_t	 i, num, datasize, entnum;
	Entry	 *entries;

	if(pathname[0] != '/') return -1;
	num = get_disknum(&pathname[1]);
	if(num == -1) return -1;
	ptr = strchr(&pathname[1], '/');
	strcpy(dirname, ptr);
	filename = strrchr(dirname, '/');
	*(filename++) = 0;
	root = get_node(num, dirname);
	if(root == 0) return -1;
	fats = load_fat(num);
	entries = (Entry*)load_data(num, fats, root, 0, CACHESIZ, &datasize);
	entnum = datasize / sizeof(Entry);
	for(i = 0;i < entnum;i++) {
		if(entries[i].name[0] == 0xff || entries[i].name[0] == 0) continue;
		if(strcmp(entries[i].name, filename) == 0) {
			if(entries[i].attr & ATTR_DIR) {
				del_all(num, entries[i].fat, fats);
			}
			for(node = entries[i].fat;next != TAIL;node = next) {
				next = fats[node];
				fats[node] = FREE;
			}
			memset((STRING)(&entries[i]), 0xff, sizeof(Entry));
			save_data(num, fats, root, 0, datasize, (STRING)entries);
			save_fat(num, fats);
			break;
		}
	}
	free_mem(meminfo, SuperPID, (STRING)fats);
	if(i == entnum) {
		free_mem(meminfo, SuperPID, (STRING)entries);
		return -1;
	} else {
		return 0;
	}
}

int32_t	minifs_disk_regist(char *device) {
	int32_t	i, n, fd;

	for(i = 0;i < DISK_NUM;i++) {
		if(disk[i].fd == 0) {
			fd = open_device(SuperPID, device, 0);
			if(fd == -1) return -1;
			disk[i].info = ioctl_device(fd, 0, DEV_INFO);
			if(disk[i].info & FILE_DEVICE) {
				disk[i].fd = fd;
				disk[i].disksize = ioctl_device(fd, 0, DISK_SIZE);
				disk[i].pagesize = ioctl_device(fd, 0, DISK_PAGE);
				strcpy(disk[i].name, device);
				n = (disk[i].disksize / disk[i].pagesize) * FATSIZEOF;
				n /= disk[i].pagesize;
				disk[i].firstfat = n;
				break;
			}
			close_device(SuperPID, fd);
		}
	}
	if(i == DISK_NUM) return -1;
	return i;
}

void minifs_init() {
	int32_t	i;

	for(i = 0;i < FD_NUM;i++) filefd[i].id = 0;
	for(i = 0;i < DISK_NUM;i++) disk[i].fd = 0;
}

