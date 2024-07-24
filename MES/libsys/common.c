#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"

Task *get_task(void) {
	register Task *task;

	TRAP1;
	asm("mov r0,%0":"=r"(task):);
	return task;
}

void s_exit() {
	register Task *task;

	task = get_task();
	task->state |= EXIT_STATE;
	TRAP0;
}
