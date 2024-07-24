/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "../syscall.h"
#include "task.h"
#define SuperPID 1
MemInfo	meminfo, tmpmem;
int32_t	stdio;
int32_t	task_sw();
void	task_init(void);
extern	Task	*curtask;
extern	int32_t	*stackptr;

int32_t init() {
	int32_t	 fsfd;
	uint32_t memaddr, memsize;
	uint32_t pgmsiz, tmpsiz, octdiv;

	stdio = open_device(SuperPID, "sci1", 0);
	get_meminfo(&memaddr, &memsize);
	if(memsize == 0) {
		write_device(stdio, "Memory not found!!", 18);
		int_enable();
		while(1);
	}
	octdiv = memsize / 8;
	tmpsiz = octdiv * 2;
	pgmsiz = octdiv * 6;
	alloc_init(&meminfo, pgmsiz, 256, (STRING)(memaddr));
	alloc_init(&tmpmem, tmpsiz, 256, (STRING)(memaddr + pgmsiz));
	set_handle(task_sw, &stackptr, &curtask);
	minifs_init();
	minifs_disk_regist("rom0");
	minifs_disk_regist("ram0");
	minifs_disk_regist("se0");
	minifs_format("ram0");
}

int main() {
	init();
	ip_init();
	task_init();
}
