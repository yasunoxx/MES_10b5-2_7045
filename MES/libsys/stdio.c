#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"
Task *get_task(void);

int s_write(int fd, char *data, int size) {
	register Task *task;

	task = get_task();
	task->req = WRITE_REQ;
	task->arg[0] = fd;
	task->arg[1] = (int)data;
	task->arg[2] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_read(int fd, char *data, int size) {
	register Task *task;

	task = get_task();
	task->req = READ_REQ;
	task->arg[0] = fd;
	task->arg[1] = (int)data;
	task->arg[2] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_open(char *name, int opt) {
	register Task *task;

	task = get_task();
	task->req = OPEN_REQ;
	task->arg[0] = (int)name;
	task->arg[1] = opt;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_close(int fd) {
	register Task *task;

	task = get_task();
	task->req = CLOSE_REQ;
	task->arg[0] = fd;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_seek(int fd, int position) {
	register Task *task;

	task = get_task();
	task->req = SEEK_REQ;
	task->arg[0] = fd;
	task->arg[1] = position;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int s_ioctl(int fd, int data, int op) {
	register Task *task;

	task = get_task();
	task->req = CONTROL_REQ;
	task->arg[0] = fd;
	task->arg[1] = data;
	task->arg[2] = op;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int delete(char *name) {
	register Task *task;

	task = get_task();
	task->req = DELETE_REQ;
	task->arg[0] = (int)name;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int getdirent(char *path, int index, char *name, int size) {
	register Task *task;

	task = get_task();
	task->req = GETDIR_REQ;
	task->arg[0] = (int)path;
	task->arg[1] = index;
	task->arg[2] = (int)name;
	task->arg[3] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

char *s_pwd() {
	register Task *task;

	task = get_task();
	return task->pwd;
}

int s_cd(char *path) {
	register Task	*task;
	char		name[2];
	int		ret;

	ret = getdirent(path, 0, name, 0);
	if(ret == -1) return -1;
	task = get_task();
	strcpy(task->pwd, path);
	return 0;
}

int s_format(char *device) {
	register Task	*task;

	task = get_task();
	task->req = FORMAT_REQ;
	task->arg[0] = (int)device;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}
