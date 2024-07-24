/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"

void init_que(QueInfo *info, uint16_t size, uint16_t number, void *ptr) {
	info->typesize = size;
	info->datasize = 0;
	info->start = 0;
	info->maxsize = number;
	info->area = ptr;
}

int32_t que_size(QueInfo *info) {
	return info->datasize;
}

int32_t push_que(QueInfo *info, void *data) {
	uint16_t index;
	STRING	 dest;

	if(info->typesize == 0) return -1;
	index = info->start + info->datasize;
	if(index >= info->maxsize) index -= info->maxsize;
	dest = info->area;
	dest = &(dest[info->typesize * index]);
	memcpy(dest, (char*)data, info->typesize);
	if(info->datasize >= info->maxsize) {
		if(info->start >= info->maxsize) {
			info->start = 0;
		} else {
			(info->start)++;
		}
	} else {
		(info->datasize)++;
	}
	return index;
}

int32_t add_que(QueInfo *info, void *data) {
	if(info->datasize >= info->maxsize) return -1;
	return push_que(info, data);
}

int32_t get_que(QueInfo *info, uint16_t pos, void *data, int32_t (*func)(void*)) {
	uint16_t index;
	STRING	 src;

	if(info->typesize == 0 || pos >= info->datasize) return -1;
	index = info->start + pos;
	if(index >= info->maxsize) index -= info->maxsize;
	src = info->area;
	src = &(src[info->typesize * index]);
	memcpy((char*)data, src, info->typesize);
	if(func != (void*)0) return (*func)(data);
	return 0;
}

int32_t set_que(QueInfo *info, uint16_t pos, void *data) {
	uint16_t index;
	STRING	 dest;

	if(info->typesize == 0 || pos >= info->datasize) return -1;
	index = info->start + pos;
	if(index >= info->maxsize) index -= info->maxsize;
	dest = info->area;
	dest = &(dest[info->typesize * index]);
	memcpy(dest, (char*)data, info->typesize);
	return 0;
}

int32_t del_que(QueInfo *info, uint16_t pos) {
	uint16_t i, sindex, dindex;
	STRING	 src, dest;

	if(info->typesize == 0 || pos >= info->datasize) return -1;
	(info->datasize)--;
	for(i = pos;i < info->datasize;i++) {
		dindex = info->start + i;
		if(dindex >= info->maxsize) dindex -= info->maxsize;
		sindex = dindex + 1;
		if(sindex >= info->maxsize) sindex -= info->maxsize;
		src = dest = info->area;
		src = &(src[info->typesize * sindex]);
		dest = &(dest[info->typesize * dindex]);
		memcpy(dest, src, info->typesize);
	}
	return 0;
}

int32_t search_que(QueInfo *info, void *compare, int32_t (*func)(void*, void*)) {
	uint16_t i, index;
	STRING	 src;

	if(info->typesize == 0 || func == (void*)0) return -1;
	for(i = 0;i < info->datasize;i++) {
		index = info->start + i;
		if(index >= info->maxsize) index -= info->maxsize;
		src = info->area;
		src = &(src[info->typesize * index]);
		if((*func)(src, compare) == 0) return i;
	}
	return -1;
}

int32_t push_fifo(QueInfo *info, void *data) {
	STRING	buf;
	int32_t	typsiz, c, b;

	if(info->datasize >= info->maxsize) return -1;
	buf = info->area;
	typsiz = info->typesize;
	for(c = info->datasize;c > 0;c--) {
		memcpy(&(buf[c * typsiz]), &(buf[(c - 1) * typsiz]), typsiz);
	}
	memcpy(buf, (char*)data, typsiz);
	(info->datasize)++;
	return 0;
}

int32_t pop_fifo(QueInfo *info, void *data) {
	STRING	buf;
	int32_t	typsiz;

	if(info->datasize == 0) return -1;
	buf = info->area;
	typsiz = info->typesize;
	(info->datasize)--;
	memcpy((char*)data, &(buf[info->datasize * typsiz]), typsiz);
	return 0;
}

STRING getptr_fifo(QueInfo *info, int32_t index) {
	STRING	buf, ptr;
	int32_t	typsiz;

	if(info->datasize == 0) return 0;
	if(index >= info->datasize) return 0;
	buf = info->area;
	typsiz = info->typesize;
	ptr = &(buf[index * typsiz]);
	return ptr;
}
