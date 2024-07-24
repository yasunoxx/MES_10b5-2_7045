// ------------------------------------------------
// 
//  Modified By
//       T.Mochizuki 04/03/09
// ------------------------------------------------
#include "../../macro.h"
#include "../../pubtype.h"
#include "../../const.h"
#include "../task.h"
#include "../ip.h"
Task *get_task(void);

int32_t ifconfig(STRING name, uint32_t ip, uint32_t mask) {
	register Task *task;

	task = get_task();
	task->req = IFCONFIG_REQ;
	task->arg[0] = (int)name;
	task->arg[1] = ip;
	task->arg[2] = mask;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t ifdown(STRING name) {
	register Task *task;

	task = get_task();
	task->req = IFDOWN_REQ;
	task->arg[0] = (int)name;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t ifup(STRING name) {
	register Task *task;

	task = get_task();
	task->req = IFUP_REQ;
	task->arg[0] = (int)name;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t ifgateway(uint32_t ip) {
	register Task *task;

	task = get_task();
	task->req = IFGATEWAY_REQ;
	task->arg[0] = ip;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t getipinfo(int32_t index, uint32_t *ip, uint32_t *mask, Byte *mac) {
	register Task *task;

	task = get_task();
	task->req = IPINFO_REQ;
	task->arg[0] = index;
	task->arg[1] = (uint32_t)ip;
	task->arg[2] = (uint32_t)mask;
	task->arg[3] = (uint32_t)mac;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t getip(int32_t index) {
	register Task *task;

	task = get_task();
	task->req = GETIP_REQ;
	task->arg[0] = index;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t get_mac_address(uint32_t req_ip, Byte *mac) {
	register Task *task;

	task = get_task();
	task->req = SEND_ARP_REQ;
	task->arg[0] = req_ip;
	task->arg[1] = (int32_t)mac;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t udp_socket() {
	register Task *task;

	task = get_task();
	task->req = UDP_SOCK_REQ;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t udp_bind(int32_t sock, void *addr) {
	register Task *task;

	task = get_task();
	task->req = UDP_BIND_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)addr;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}	

int32_t udp_free(int32_t sock) {
	register Task *task;

	task = get_task();
	task->req = UDP_FREE_REQ;
	task->arg[0] = sock;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t recvfrom(int32_t sock, STRING data, int32_t size, struct sockaddr *from) {
	register Task   *task;

	task = get_task();
	task->req = UDP_RECV_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)data;
	task->arg[2] = size;
	task->arg[3] = (int32_t)from;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t sendto(int32_t sock, STRING data, int32_t size, struct sockaddr *target) {
	register Task   *task;
	// del T.Mochizuki
	//Byte		mac[6];
	// if(get_mac_address(target->sin_addr, mac) == -1) return -1;
	task = get_task();
	task->req = UDP_SEND_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)data;
	task->arg[2] = size;
	task->arg[3] = (int32_t)target;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t tcp_socket() {
	register Task *task;

	task = get_task();
	task->req = TCP_SOCK_REQ;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t tcp_listen(int32_t sock, void *addr) {
	register Task *task;

	task = get_task();
	task->req = TCP_LISTEN_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)addr;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}	

int32_t tcp_free(int32_t sock) {
	register Task *task;

	task = get_task();
	task->req = TCP_FREE_REQ;
	task->arg[0] = sock;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t tcp_accept(int32_t sock, void *addr) {
	register Task *task;

	task = get_task();
	task->req = TCP_ACCEPT_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)addr;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}	

int32_t tcp_connect(int32_t sock, void *addr) {
        //struct sockaddr *scaddr;
        //      Byte mac[6];
	register Task *task;
	//	scaddr = (struct sockaddr *)addr;
	//  if ( get_mac_address(scaddr->sin_addr, mac) != 0 ) return -1;
	task = get_task();
	task->req = TCP_CONN_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)addr;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}	

int32_t tcp_recv(int32_t sock, STRING data, int32_t size) {
	register Task *task;

	task = get_task();
	task->req = TCP_RECV_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)data;
	task->arg[2] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}

int32_t tcp_send(int32_t sock, STRING data, int32_t size) {
	register Task *task;

	task = get_task();
	task->req = TCP_SEND_REQ;
	task->arg[0] = sock;
	task->arg[1] = (int32_t)data;
	task->arg[2] = size;
	task->state |= REQ_STATE;
	TRAP0;
	return task->retval;
}	

int32_t tcp_bind(int32_t sock, void *addr) {
        register Task *task;

        task = get_task();
        task->req = TCP_BIND_REQ;
        task->arg[0] = sock;
        task->arg[1] = (int32_t)addr;
        task->state |= REQ_STATE;
        TRAP0;
        return task->retval;
}
