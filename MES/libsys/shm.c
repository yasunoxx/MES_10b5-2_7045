#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"
Task *get_task(void);

int	shmget(int key, int size) {
	register Task *task;

	task = get_task();
	task->req = SHM_GET_REQ;
	task->arg[0] = key;
	task->arg[1] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

char	*shmat(int id) {
	register Task *task;

	task = get_task();
	task->req = SHM_AT_REQ;
	task->arg[0] = id;
	task->state |= REQ_STATE;
	TRAP0;
	return (char*)task->retval;
}

int	shmdt(int id) {
	register Task *task;

	task = get_task();
	task->req = SHM_DT_REQ;
	task->arg[0] = id;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}
