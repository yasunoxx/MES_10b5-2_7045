/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"

#ifdef SH_7045
  #include "rom7045.h"
  #define CODE_AREA	((void*)0xffff6000)
  #define BLK_SIZ	128
#endif

int32_t set_flush_freq(int32_t f) {
	return 0;
}

int32_t load_program() {
	return 0;
}

int32_t write_program(STRING target, STRING source, int32_t size) {
	int32_t	i, mod, ccrsv;
	Byte	buffer[128], *ptr;
	Byte	(*write_flush)(char*, char*);

	write_flush = CODE_AREA;
	ptr = (char*)((int)target & ~0x7f);
	mod = (int)target % 128;
	if(size + mod > 128) return -1;
	for(i = 0;i < 128;i++) buffer[i] = ptr[i];
	for(i = 0;i < size;i++) buffer[i + mod] = source[i];
	ccrsv = int_disable();
	memcpy(CODE_AREA, writecode, 0x400);
	for(i = 0;i < 128;i += BLK_SIZ) {
		if((*write_flush)(&(ptr[i]), &(buffer[i])) != 0) {
			set_flag(ccrsv);
			return -1;
		}
	}
	bzero(CODE_AREA, 0x400);
	set_flag(ccrsv);
	return 0;
}

int erase_program(char *address) {
	Byte	(*erase_flush)(char*);
	int32_t	ret, ccrsv;

	erase_flush = CODE_AREA;
	ccrsv = int_disable();
	memcpy(CODE_AREA, erasecode, 0x400);
	ret = (*erase_flush)(address);
	bzero(CODE_AREA, 0x400);
	set_flag(ccrsv);
	return (ret == 0) ? 0 : -1;
}
