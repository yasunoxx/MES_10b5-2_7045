/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"

void ring_init(RingInfo *info, uint32_t size, STRING ptr) {
	int	i;

	info->maxsize = size;
	info->ptr = ptr;
	info->start = 0;
	info->datasize = 0;
	bzero(ptr, size);
}

int write_ring(RingInfo *info, STRING data, uint32_t size) {
	uint32_t	s, i;

	if(info->maxsize == 0 || size > info->maxsize) return -1;
	i = info->start + info->datasize;
	if(i >= info->maxsize) i -= info->maxsize;
	if((i + size) > info->maxsize) {
		s = info->maxsize - i;
		memcpy(&(info->ptr[i]), data, s); 
		memcpy(info->ptr, &(data[s]), size - s);
	} else {
		memcpy(&(info->ptr[i]), data, size); 
	}
	info->datasize += size;
	if(info->datasize > info->maxsize) {
		info->start += info->datasize - info->maxsize;
		info->datasize = info->maxsize;
	}
	if(info->start >= info->maxsize) info->start -= info->maxsize;
	return size;
}

int read_ring(RingInfo *info, STRING data, uint32_t size) {
	uint32_t count, s;

	if(info->maxsize == 0 || size > info->maxsize) return -1;
	if(info->datasize == 0) return 0;
	count = (size > info->datasize) ? info->datasize : size;
	if((info->start + count) > info->maxsize) {
		s = info->maxsize - info->start;
		memcpy(data, &(info->ptr[info->start]), s); 
		memcpy(&(data[s]), info->ptr, count - s); 
	} else {
		memcpy(data, &(info->ptr[info->start]), count); 
	}
	info->datasize -= count;
	info->start += count;
	if(info->start >= info->maxsize) info->start -= info->maxsize;
	return count;
}
