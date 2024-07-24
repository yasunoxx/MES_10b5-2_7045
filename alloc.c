/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"

#define FREE	0
#define CONT	-1

void alloc_init(MemInfo *info, uint32_t memsize, uint32_t secsize, STRING memarea) {
	info->memsize = memsize;
	info->secsize = secsize;
	info->memarea = memarea;
	info->secstart = (memsize * sizeof(uint16_t)) / (secsize * secsize) + 1;
	info->secend = memsize / secsize;
	if((info->secend * sizeof(uint16_t)) > (info->secstart * info->secsize)) (info->secstart)++;
	bzero(info->memarea, info->secend * sizeof(uint16_t));
}

STRING alloc_mem(MemInfo info, uint16_t id, uint32_t memsize) {
	uint32_t	i, c, count, a;
	uint16_t	*mat;
	Byte		*ptr;

	mat = (uint16_t*)info.memarea;
	count = memsize / info.secsize;
	if((memsize % info.secsize) > 0) count++;
	c = 0;
	for(i = info.secstart;i < info.secend;i++) {
		if(mat[i] == FREE) c++;
		else c = 0;
		if(c == count) {
			a = i - c + 1;
			ptr = &(info.memarea[a * info.secsize]);
			mat[a] = id;
			for(i = 1;i < c;i++) mat[a + i] = CONT;
			return ptr;
		}
	}
	return 0;
}

int free_mem(MemInfo info, uint16_t id, STRING ptr) {
	uint32_t	i, index;
	uint16_t	*mat;

	mat = (uint16_t*)info.memarea;
	if(ptr < &(info.memarea[info.secstart * info.secsize])) return -1;
	if(ptr >= &(info.memarea[info.secend * info.secsize])) return -1;
	index = ((uint32_t)ptr - (uint32_t)info.memarea) / info.secsize;
	if(mat[index] != id) return -1;
	mat[index++] = FREE;
	for(i = index;i < info.secend;i++) {
		if(mat[i] == (uint16_t)CONT) mat[i] = FREE;
		else break;
	}
	return 0;
}

void free_idmem(MemInfo info, uint16_t id) {
	uint32_t	i, flag;
	uint16_t	*mat;
	STRING		ptr;

	mat = (uint16_t*)info.memarea;
	flag = 0;
	for(i = info.secstart;i < info.secend;i++) {
		if(mat[i] == id) {
			mat[i] = FREE;
			flag = 1;
		} else if(mat[i] == (uint16_t)CONT && flag == 1) {
			mat[i] = FREE;
		} else {
			flag = 0;
		}
	}
}

int change_id_mem(MemInfo info, uint16_t oldid, uint16_t newid, STRING ptr) {
	uint32_t	index;
	uint16_t	*mat;

	mat = (uint16_t*)info.memarea;
	if(ptr < &(info.memarea[info.secstart * info.secsize])) return -1;
	if(ptr >= &(info.memarea[info.secend * info.secsize])) return -1;
	index = ((uint32_t)ptr - (uint32_t)info.memarea) / info.secsize;
	if(mat[index] != oldid) return -1;
	mat[index] = newid;
	return 0;
}
