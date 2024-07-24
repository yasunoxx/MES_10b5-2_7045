#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"
Task *get_task(void);

char* s_malloc(int size) {
	register Task *task;

	task = get_task();
	task->req = ALLOC_REQ;
	task->arg[0] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return (char*)task->retval;
}

int s_free(char *ptr) {
	register Task *task;

	task = get_task();
	task->req = FREE_REQ;
	task->arg[0] = (int)ptr;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}
