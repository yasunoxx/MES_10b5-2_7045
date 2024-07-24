/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/*  Modified By				*/
/*     T.Mochizuki 04/03/09             */
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "task.h"
#include "ip.h"
#include "map.h"
#define ARP_REQ_COUNT    4000
extern	MemInfo	meminfo, tmpmem;
extern	Task		*curtask;
void	start_task(void);
int32_t	tcpsocket(int16_t);
int32_t tcpfree(int16_t, int32_t);
int32_t tcpfreeid(int16_t);
int32_t tcplisten(int16_t, int32_t, struct sockaddr*);
int32_t tcpsend(int16_t, uint32_t, STRING, int32_t);
int32_t tcp_setid(uint16_t, uint32_t);
int32_t tcpgetdstIP(uint32_t ,uint32_t*);
Byte tcp_getstate(uint32_t);
int32_t tcpbind(int16_t, uint32_t, struct sockaddr*);
int32_t tcpconnect(int16_t, uint32_t, struct sockaddr*);
int32_t get_mac(uint32_t*, Byte*);

static int32_t _exec(int32_t pid, int32_t argc, STRING *argv, int32_t *stacksize) {
	int32_t	i, fd, coffsize, pgmsize, num, *ptr, newsp, ret, drive;
	STRING	pgmarea, coffarea;
	void	(*func)(int32_t, STRING*);

	fd = open_file(pid, argv[0], READ_FILE);
	if(fd == -1) return 0;
	coffsize = ioctl_file(fd, 0, FILE_SIZE);
	coffarea = alloc_mem(tmpmem, SuperPID, coffsize);
	if(coffarea == 0) return 0;
	read_file(fd, coffarea, coffsize);
	close_file(pid, fd);
	pgmsize = get_program_size(coffarea);
	if(pgmsize == 0) {
		free_mem(tmpmem, SuperPID, coffarea);
		return 0;
	}
	*stacksize = stack_size(coffarea);
	pgmarea = alloc_mem(meminfo, SuperPID, pgmsize);
	if(pgmarea == 0) {
		free_mem(tmpmem, SuperPID, coffarea);
		return 0;
	}
	bzero(pgmarea, pgmsize);
	ret = coff2bin(pgmarea, coffarea, symbol, symptr);
	free_mem(tmpmem, SuperPID, coffarea);
	if(ret != 0) {
		free_mem(meminfo, SuperPID, pgmarea);
		pgmarea = 0;
	}
	return (int32_t)pgmarea;
}

static int32_t get_stdfd(Task *task, int32_t fd) {
	int32_t		ret;
	
	switch(fd) {
	case STDOUT_FD:
		ret = task->stdout;
		break;
	case STDIN_FD:
		ret = task->stdin;
		break;
	case STDERR_FD:
		ret = task->stderr;
		break;
	default:
		ret = fd;
	}
	return ret;
}
// -1 : Not found,or No net
//  0 : Found
int32_t arp_request(Task *task,uint32_t req_ip,int32_t count,int32_t retry) {
  uint32_t ip;
  int32_t ret;
  Byte mac[6];
  if(task->pid == 0) return -1;

  ip = req_ip;
  ret = get_mac(&ip,mac);
  switch(ret) {
  case -1: // Not found
    if ( retry > 0 ) {
      send_arp(ip);
      task->net_count = count*retry;
      task->retry = count;
    } else {
      if( task->net_count != 0 && (task->net_count-1) % task->retry == 0 ) {
	send_arp(ip);
      }
    }
    break;
  case 0: // No Net
    task->retry = 0;
    task->net_count = 0;
    ret = -1;
    break;
  case 1: // Found
    ret = 0;
    task->retry = 0;
    task->net_count = 0;
    break;
  }
  return ret;
}
int32_t request(Task *task) {
	int32_t	ret, func, stacksize, drive, fd;
	struct Entry *ent;
	struct sockaddr *target;
	Task	*taskptr;
	Byte	tcp_state;
	uint32_t ip;

	if(task->pid == 0) return -1;
	switch(task->req) {
	case WRITE_REQ:
		ret = write_file(get_stdfd(task, task->arg[0]), (STRING)task->arg[1], task->arg[2]); 
		break;
	case READ_REQ:
		ret = read_file(get_stdfd(task, task->arg[0]), (STRING)task->arg[1], task->arg[2]); 
		break;
	case OPEN_REQ:
		ret = open_file(task->pid, (STRING)task->arg[0], task->arg[1]);
		break;
	case CLOSE_REQ:
		ret = close_file(task->pid, task->arg[0]);
		break;
	case SEEK_REQ:
		ret = seek_file(get_stdfd(task, task->arg[0]), task->arg[1]);
		break;
	case CONTROL_REQ:
		ret = ioctl_file(get_stdfd(task, task->arg[0]), task->arg[1], task->arg[2]);
		break;
	case DELETE_REQ:
		ret = delete_file(task->pid, task->arg[0]);
		break;
	case GETDIR_REQ:
		ret = get_dirent(task->pid, (STRING)task->arg[0], task->arg[1], (STRING)task->arg[2], task->arg[3]);
		break;
	case EXEC_REQ:
		func = _exec(task->pid, task->arg[0], (STRING*)task->arg[1], &stacksize);
		stacksize = (stacksize < HEAPSIZE) ? HEAPSIZE : stacksize;
		if(func != 0) {
			ret = attach_task(stacksize, func, task->arg[0], (STRING*)task->arg[1],
					  task->pid, task->arg[2], task->arg[3]);
		} else {
			ret = -1;
		}
		break;
	case KILL_REQ:
		ret = -1;
		for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
			if(taskptr->pid == task->arg[0]) {
				taskptr->state |= EXIT_STATE;
				ret = 0;
				break;
			}
		}
		break;
	case WAIT_REQ:
		ret = -1;
		for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
			if(taskptr->pid == task->arg[0]) {
				ret = 0;
				task->state |= WAIT_STATE;
				task->wait_pid = task->arg[0];
				break;
			}
		}
		break;
	case STDOUT_REQ:
		ret = -1;
		for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
			if(taskptr->pid == task->arg[0]) {
				taskptr->stdout = task->arg[1];
				ret = 0;
				break;
			}
		}
		break;
	case STDIN_REQ:
		ret = -1;
		for(taskptr = curtask->next;taskptr != curtask;taskptr = taskptr->next) {
			if(taskptr->pid == task->arg[0]) {
				taskptr->stdin = task->arg[1];
				ret = 0;
				break;
			}
		}
		break;
	case FORMAT_REQ:
		ret = format((STRING)task->arg[0]);
		break;
	case ALLOC_REQ:
		ret = (int)alloc_mem(meminfo, task->pid, task->arg[0]);
		break;
	case FREE_REQ:
		ret = free_mem(meminfo, task->pid, (STRING)task->arg[0]);
		break;
	case IFCONFIG_REQ:
		ret = if_config((STRING)task->arg[0], task->arg[1], task->arg[2]);
		break;
	case IFDOWN_REQ:
		ret = if_down((STRING)task->arg[0]);
		break;
	case IPINFO_REQ:
		ret = get_ipinfo(task->arg[0], (uint32_t*)task->arg[1], (uint32_t*)task->arg[2], (uint32_t*)task->arg[3]);
		break;
	case GETIP_REQ:
		ret = get_ip(task->arg[0]);
		break;
	case IFUP_REQ:
		ret = if_up((STRING)task->arg[0]);
		break;
	case IFGATEWAY_REQ:
		ret = if_gateway(task->arg[0]);
		break;
	case SEND_ARP_REQ:
	  ret = arp_request(task,task->arg[0],ARP_REQ_COUNT,3);
		break;
	case UDP_SOCK_REQ:
		ret = udpsocket(task->pid);
		break;
	case UDP_BIND_REQ:
		ret = udpbind(task->pid, task->arg[0], (void*)task->arg[1]);
		break;
	case UDP_FREE_REQ:
		ret = udpfree(task->pid, task->arg[0]);
		break;
	case UDP_RECV_REQ:
		ret = udp_recv(task->arg[0], (STRING)task->arg[1], task->arg[2], (void*)task->arg[3]);
		if(ret == -1) task->net_count = 8000; 
		break;
	case UDP_SEND_REQ:
	  target = (struct sockaddr *)task->arg[3];
	  ret = arp_request(task,target->sin_addr,ARP_REQ_COUNT,3); 
	  if ( ret == 0 ) { 
	    ret = udp_send(task->arg[0], (STRING)task->arg[1], task->arg[2], (void*)task->arg[3]);
	  }
	  break;
	case TCP_SOCK_REQ:
		ret = tcpsocket(task->pid);
		break;
	case TCP_LISTEN_REQ:
		ret = tcplisten(task->pid, task->arg[0], (void*)task->arg[1]);
		break;
	case TCP_BIND_REQ:
		ret = tcpbind(task->pid, task->arg[0], (void*)task->arg[1]);
		break;
	case TCP_CONN_REQ:
	  target = (struct sockaddr *)task->arg[1];
	  ret = arp_request(task,target->sin_addr,ARP_REQ_COUNT,3);
	  if ( ret == 0 ) { 
	    ret = tcpconnect(task->pid, task->arg[0], (void*)task->arg[1]);
	    task->net_count = 1000;
	  }
	  break;
	case TCP_FREE_REQ:
		ret = tcpfree(task->pid, task->arg[0]);
		break;
	case TCP_SEND_REQ:
	  if ( tcpgetdstIP(task->arg[0],&ip) == 0 ) {
	    ret = arp_request(task,ip,ARP_REQ_COUNT,3);
	    if ( ret == 0 ) { 
	      ret = tcpsend(task->pid, task->arg[0], (STRING)(task->arg[1]), task->arg[2]);
	    }
	  }
	  break;
	case TCP_ACCEPT_REQ:
		tcp_state = tcp_getstate(task->arg[0]);
		if(tcp_state == TCP_STATE_LISTENING) {
			task->state |= NET_STATE;
			tcp_setid(task->pid, task->arg[0]);
		} else {
			ret = -1;
		}
		break;
	case TCP_RECV_REQ:
		ret = tcprecv(task->pid, task->arg[0], (STRING)(task->arg[1]), task->arg[2]);
		break;
	case SHM_GET_REQ:
		ret = shm_get(task->arg[0], task->arg[1]);
		break;
	case SHM_AT_REQ:
		ret = (int32_t)shm_at(task->arg[0]);
		break;
	case SHM_DT_REQ:
		ret = shm_dt(task->arg[0]);
		break;
	default:
		ret = -1;
	}
	task->state &= ~REQ_STATE;
	task->retval = ret;
	return 0;
}

int32_t net_request(Task *task) {
	Task	 *taskptr;
	int32_t	 ret;
	uint32_t req_ip,ip;
	STRING	 mac;
	struct sockaddr *target;
	Byte state;


	if(task->pid == 0) return -1;
	switch(task->req) {
	case SEND_ARP_REQ:
	  ret = arp_request(task,task->arg[0],ARP_REQ_COUNT,-1); 
	  break;
	case UDP_SEND_REQ: 
	  target = (struct sockaddr *)task->arg[3];
	  ret = arp_request(task,target->sin_addr,ARP_REQ_COUNT,-1); 
	  if ( ret == 0 ) { 
	    ret = udp_send(task->arg[0], (STRING)task->arg[1], task->arg[2], (void*)task->arg[3]);
	  }
	  break;
	case TCP_CONN_REQ:
	  state = tcp_getstate(task->arg[0]);
	  if ( state == TCP_STATE_CONNECTED ) {
	    task->net_count = 0;
	  } else if ( state == TCP_STATE_RESERVED || 
                      state == TCP_STATE_LISTENING || 
                      state == TCP_STATE_CLOSED  ) {
	    target = (struct sockaddr *)task->arg[1];
	    ret = arp_request(task,target->sin_addr,ARP_REQ_COUNT,-1); 
	    if ( ret == 0 ) { 
	      ret = tcpconnect(task->pid, task->arg[0], (void*)task->arg[1]);
	      task->net_count = 1000;
	    }
	  }
	  break;
	case TCP_SEND_REQ:
	  if ( tcpgetdstIP(task->arg[0],&ip) == 0 ) {
	    ret = arp_request(task,ip,ARP_REQ_COUNT,-1); 
	    if ( ret == 0 ) { 
	      ret = tcpsend(task->pid, task->arg[0], (STRING)(task->arg[1]), task->arg[2]);
	    }
	  } else {
	    ret = -1;
	  }
	default:
		ret = -1;
	}
	task->retval = ret;
	return 0;
}
