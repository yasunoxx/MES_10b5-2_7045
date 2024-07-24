#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"
Task *get_task(void);

int execute(int argc, char** argv) {
	register Task *task;

	task = get_task();
	task->req = EXEC_REQ;
	task->arg[0] = argc;
	task->arg[1] = (int)argv;
	task->arg[2] = 0;
	task->arg[3] = 0;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int exec_redirect(int argc, char** argv, int stdout, int stdin) {
	register Task *task;

	task = get_task();
	task->req = EXEC_REQ;
	task->arg[0] = argc;
	task->arg[1] = (int)argv;
	task->arg[2] = stdout;
	task->arg[3] = stdin;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_kill(int pid) {
	register Task *task;

	task = get_task();
	task->req = KILL_REQ;
	task->arg[0] = pid;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_wait(int pid) {
	register Task *task;

	task = get_task();
	task->req = WAIT_REQ;
	task->arg[0] = pid;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int get_pid() {
	register Task *task;

	task = get_task();
	return task->pid;
}

void sleep(int ms) {
	register Task *task;

	task = get_task();
	task->count = ms;
	TRAP0;
}

int set_stdout(int pid, int fd) {
	register Task *task;

	task = get_task();
	task->req = STDOUT_REQ;
	task->arg[0] = pid;
	task->arg[1] = fd;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int set_stdin(int pid, int fd) {
	register Task *task;

	task = get_task();
	task->req = STDIN_REQ;
	task->arg[0] = pid;
	task->arg[1] = fd;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int set_timer(void (*func)(), int time) {
	register Task *task;

	task = get_task();
	task->interval = time;
	task->sigfunc = func;
	return 0;
}
