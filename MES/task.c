/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "task.h"
int32_t	*stackptr;
Task	*tcb_table, *curtask;
int32_t	sys_count;
extern	int32_t	 stdio;
extern	MemInfo	 meminfo, tmpmem;
static	uint32_t counter, pid_count, time_count;
void	check_flagroot_index();
void	tcp_interval();
int32_t	net_request(Task*);

static void start_timer() {
	volatile char	w;
	int32_t		count;

	count = (int)TCNT_RD & 0xff;
	if(count < counter) count += 256;
	time_count += count - counter;
	time_count++;
	TCNT_WR = 0x5a00 | counter;
	w = TCSR_RD;
	TCSR_WR = 0xa500 | 0x3c;	/* CLK x 256 */
}

static void init_timer() {
	time_count = 0;
	counter = ((20 * 1000) >> 8) & 0xff;
	counter = (256 - counter) & 0xff;
	start_timer();
}

static int32_t argsize(int32_t argc, STRING *argv) {
	int32_t	i, size;

	size = 0;
	for(i = 0;i < argc;i++) size += strlen(argv[i]) + 1;
	size += sizeof(STRING) * (argc + 1);
	return size;
}

static void argcopy(int32_t argc, STRING dest, STRING *src) {
	int32_t	i, n, size;
	STRING	ptr, *argv;

	argv = (STRING*)dest;
	ptr = &(dest[sizeof(STRING) * (argc + 1)]);
	for(i = 0;i < argc;i++) {
		strcpy(ptr, src[i]);
		argv[i] = ptr;
		n = strlen(ptr);
		ptr = &(ptr[n + 1]);
	}
	argv[i] = 0;
}

int32_t attach_task(int32_t heap_size, int32_t func, int32_t argc, STRING *argv, int32_t ppid, int32_t stdout, int32_t stdin) {
	int32_t	t, *sp_ptr, memsize, as;
	STRING	memptr;
	Task	*taskptr;

	for(t = 0;t < MAX_TASK;t++) {
		if(tcb_table[t].pid == 0) break;
	}
	if(t == MAX_TASK) return -1;
	pid_count++;
	if(pid_count > 30000) pid_count = 3;
	for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
		if(taskptr->pid == pid_count) pid_count++;
	}
	as = argsize(argc, argv);
	memsize = heap_size + as + 64;
	memptr = alloc_mem(meminfo, pid_count, memsize);
	if(memptr == 0) return -1;
	bzero(memptr, memsize);
	tcb_table[t].pwd = &memptr[as];
	strcpy(tcb_table[t].pwd, "/ram0/");
	tcb_table[t].prev = curtask;
	tcb_table[t].next = curtask->next;
	curtask->next->prev = &(tcb_table[t]);
	curtask->next = &(tcb_table[t]);
	tcb_table[t].ppid = ppid;
	tcb_table[t].pid = pid_count;
	tcb_table[t].state = 0;
	tcb_table[t].count = 0;
	tcb_table[t].net_count = 0;
	tcb_table[t].interval = 0;
	tcb_table[t].sigfunc = 0;
	tcb_table[t].stdout = (stdout == 0) ? stdio : stdout;
	tcb_table[t].stdin = (stdin == 0) ? stdio : stdin;
	tcb_table[t].stderr = stdio;
	tcb_table[t].wait_pid = 0;
	sp_ptr = (int32_t*)(&memptr[(memsize >> 2) << 2]);
	sp_ptr = &(sp_ptr[STACK_NUM]);
	sp_ptr[PR_REG] = func;
	sp_ptr[PC_REG] = func;
	sp_ptr[ARGC] = argc;
	sp_ptr[ARGV] = (int32_t)memptr;
	tcb_table[t].sp = sp_ptr;
	argcopy(argc, memptr, argv);
	change_id_mem(meminfo, SuperPID, tcb_table[t].pid, (STRING)func);
	return tcb_table[t].pid;
}

int32_t dettach_task(Task *task) {
	Task	*taskptr;

	if(task->pid == 0) return -1;
	for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
		if((taskptr->state & WAIT_STATE) && (taskptr->wait_pid == task->pid)) {
			taskptr->state &= ~WAIT_STATE;
		}
	}
	free_idmem(meminfo, task->pid);
	free_idmem(tmpmem, task->pid);
	udpfreeid(task->pid);
	tcpfreeid(task->pid);
	task->pid = 0;
	task->prev->next = task->next;
	task->next->prev = task->prev;
	return 0;
}

void initial() {
	static STRING	ptrs[2];
	static Byte	name[16];
	static int64_t	sys_count_ms;
	volatile char	w;
	Task		*taskptr;
	int32_t		c, *sp_ptr;

	strcpy(name, "/rom0/shell");
	ptrs[0] = name;
	ptrs[1] = 0;
	curtask->req = EXEC_REQ;
	curtask->arg[0] = 1;
	curtask->arg[1] = (int32_t)ptrs;
	request(curtask);
	if(curtask->retval == -1) {
		strcpy(name, "/rom0/netsh");
		ptrs[0] = name;
		ptrs[1] = 0;
		curtask->req = EXEC_REQ;
		curtask->arg[0] = 1;
		curtask->arg[1] = (int32_t)ptrs;
		request(curtask);
		if(curtask->retval == -1) {
			write_device(stdio, "Shell not found, system halt.\n", 31);
			while(1);
		}
	}
	sys_count_ms = 0;
	while(1) {
		int_disable();
		for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
			if(taskptr->state & EXIT_STATE) {
				dettach_task(taskptr);
				taskptr = curtask->next;
				continue;
			}
			if(taskptr->state & REQ_STATE) request(taskptr);
			if(taskptr->net_count > 0) {
				(taskptr->net_count)--;
				net_request(taskptr);
			}
			if(taskptr->count > 0) {
				taskptr->count -= time_count / counter;
				if(taskptr->count <= 0) taskptr->count = 0;
			}
			if(taskptr->interval > 0) {
				taskptr->interval -= time_count / counter;
				if(taskptr->interval <= 0) {
					sp_ptr[PR_REG] = sp_ptr[PC_REG];
					sp_ptr[PC_REG] = (int32_t)(taskptr->sigfunc);
					taskptr->interval = 0;
				}
			}
			sys_count_ms += time_count / counter;
		}
		ip_request();
		time_count -= (time_count / counter) * counter;
		if(sys_count != sys_count_ms / 1000) {
			check_flagroot_index();
			tcp_interval();
		}
		sys_count = sys_count_ms / 1000;
		int_enable();
		TRAP0;
	}
}

void task_init() {
	int32_t	*sp_ptr, fd, size;
	int32_t	task_sw();

	pid_count = 0;
	tcb_table = (Task*)alloc_mem(meminfo, SuperPID, sizeof(Task) * MAX_TASK);
	bzero((STRING)tcb_table, sizeof(Task) * MAX_TASK);
	curtask = tcb_table;
	curtask->prev = curtask;
	curtask->next = curtask;
	attach_task(0x5000, (int32_t)initial, 0, 0, SuperPID, 0, 0);
	stackptr = curtask->sp;
	init_timer();
	TRAP2;
}

Task* get_taskptr(int32_t id) {
	Task	*taskptr;

	for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
		if(taskptr->pid == id) return taskptr;
	}
	if(taskptr->pid == id) return taskptr;
	return 0;
}

void task_sw() {
	volatile char	w;

	curtask->sp = stackptr;
	do {
		curtask = curtask->next;
	} while(curtask->state != 0 || curtask->count > 0 || curtask->net_count > 0); 
	stackptr = curtask->sp;
	start_timer();
}
