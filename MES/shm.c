/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
extern	MemInfo	 meminfo;
typedef struct {
	uint32_t	key;
	uint32_t	size;
	STRING		ptr;
} ShmInfo;
#define SuperPID 1
#define SHM_NUM	 64
static ShmInfo sharemem[SHM_NUM];

void shm_init() {
	int32_t	i;

	for(i = 0;i < SHM_NUM;i++) {
		sharemem[i].size = 0;
		sharemem[i].key = 0;
		sharemem[i].ptr = 0;
	}
}

int32_t shm_get(uint32_t key, int32_t size) {
	int32_t	i;

	for(i = 0;i < SHM_NUM;i++) {
		if(sharemem[i].key == key) return i;
	}
	for(i = 0;i < SHM_NUM;i++) {
		if(sharemem[i].size == 0) {
			sharemem[i].key = key;
			sharemem[i].size = size;
			return i;
		}
	}
	return -1;
}

STRING shm_at(uint32_t id) {
	STRING	ptr;

	if(id >= SHM_NUM ||
	   sharemem[id].size == 0 ||
	   sharemem[id].key == 0) return 0;
	if(sharemem[id].ptr == 0) {
		ptr = alloc_mem(meminfo, SuperPID, sharemem[id].size);
		if(ptr == 0) return 0;
		sharemem[id].ptr = ptr;
	}
	return sharemem[id].ptr;
}

int shm_dt(uint32_t id) {
	if(id >= SHM_NUM ||
	   sharemem[id].size == 0 ||
	   sharemem[id].key == 0 ||
	   sharemem[id].ptr == 0) return -1;
	free_mem(meminfo, SuperPID, sharemem[id].ptr);
	sharemem[id].size = 0;
	sharemem[id].key = 0;
	sharemem[id].ptr = 0;
	return 0;
}
