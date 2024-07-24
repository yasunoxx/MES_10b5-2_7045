/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "task.h"
#include "ip.h"
#define SuperPID 1
#define UDPNUM	128

typedef struct {
	void	 *packet;
	uint32_t src_ip;
	uint32_t sin_addr;
	uint16_t sin_port;
	uint16_t id;
	uint16_t flag;
} bindinfo;
#define NO_BIND	1
#define BINDED	2

static bindinfo	udp_table[UDPNUM];
extern Task	*curtask;
extern MemInfo	meminfo, tmpmem;
uint16_t make_crc(STRING, int32_t);
Task* get_taskptr(int32_t);

void udp_init() {
	int32_t		c;

	for(c = 0;c < UDPNUM;c++) {
		udp_table[c].sin_addr = 0;
		udp_table[c].sin_port = 0;
		udp_table[c].id = 0;
		udp_table[c].flag = 0;
		udp_table[c].packet = 0;
	}
}

int32_t udpsocket(int16_t id) {
	int32_t		c;

	for(c = 0;c < UDPNUM;c++) {
		if(udp_table[c].id == 0) {
			udp_table[c].id = id;
			udp_table[c].packet = 0;
			udp_table[c].flag = NO_BIND;
			return c | UDP_SOCK;
		}
	}
	return -1;
}	

int32_t udpbind(int16_t id, int32_t soc, struct sockaddr *bind) {
	int32_t	s, sock;

	sock = soc & SOCK_MASK;
	if(!(soc & UDP_SOCK) || sock >= UDPNUM) return -1;
	if(udp_table[sock].id != id) return -1;
	for(s = 0;s < UDPNUM;s++) {
		if(udp_table[s].flag & BINDED &&
		   udp_table[s].sin_addr == bind->sin_addr &&
		   udp_table[s].sin_port == bind->sin_port) return -1;
	}
	udp_table[sock].sin_addr = bind->sin_addr;
	udp_table[sock].sin_port = bind->sin_port;
	udp_table[sock].flag &= ~NO_BIND;
	udp_table[sock].flag |= BINDED;
	return 0;
}

int32_t udpfree(int16_t id, int32_t soc) {
	int32_t	sock;

	sock = soc & SOCK_MASK;
	if(!(soc & UDP_SOCK) || sock >= UDPNUM) return -1;
	if(udp_table[sock].id != id) return -1;
	udp_table[sock].sin_addr = 0;
	udp_table[sock].sin_port = 0;
	udp_table[sock].id = 0;
	udp_table[sock].flag = 0;
	if(udp_table[sock].packet) {
		free_mem(meminfo, id, udp_table[sock].packet);
		udp_table[sock].packet = 0;
	}
	return 0;
}

int32_t udpfreeid(int16_t id) {
	int32_t		c;

	for(c = 0;c < UDPNUM;c++) {
		if(udp_table[c].id == id) {
			udp_table[c].sin_addr = 0;
			udp_table[c].sin_port = 0;
			udp_table[c].id = 0;
			udp_table[c].flag = 0;
			if(udp_table[c].packet) {
				free_mem(meminfo, id, udp_table[c].packet);
				udp_table[c].packet = 0;
			}
		}
	}
	return 0;
}

int32_t udp_send(int32_t soc, STRING data, int32_t udpsize, struct sockaddr *target) {
	UDP_HDR	 *udphdr;
	Ether	 *ether_buf;
	STRING	 ptr, packet;
	int32_t	 index, datasize, fd, pktsize, size, sock;
	uint16_t myport;

	sock = soc & SOCK_MASK;
	if(!(soc & UDP_SOCK) || sock >= UDPNUM) return -1;
	if(udp_table[sock].flag & BINDED) myport = udp_table[sock].sin_port;
	else myport = sock + 0x8000;
	pktsize	= sizeof(Ether) + sizeof(IP_HDR) + sizeof(UDP_HDR) + udpsize;
	packet = alloc_mem(tmpmem, SuperPID, pktsize);
	if(packet == 0) return -1;
	bzero(packet, sizeof(Ether) + sizeof(IP_HDR) + sizeof(UDP_HDR));
	memcpy(&(packet[sizeof(Ether) + sizeof(IP_HDR) + sizeof(UDP_HDR)]), data, udpsize);
	ether_buf = (Ether*)packet;
	for(index = 0;index < IPNUM;index++) {
		if((fd = index2fd(index)) != -1) {
			ip_opt_header(ether_buf, 0, 0);
			size = ip_header(fd, ether_buf, target->sin_addr, UDP, sizeof(UDP_HDR) + udpsize);
			ptr = (STRING)&ether_buf[1];
			ptr = &(ptr[sizeof(IP_HDR)]);
			udphdr = (UDP_HDR*)ptr;
			udphdr->src_port = myport;
			udphdr->dest_port = target->sin_port;
			udphdr->data_size = sizeof(UDP_HDR) + udpsize;
			udphdr->checksum = 0;
			write_ip_packet(fd, (STRING)ether_buf, pktsize);
		}
	}
	free_mem(tmpmem, SuperPID, packet);
	return udpsize;
}

int32_t udp_recv(int32_t soc, STRING data, int32_t udpsize, struct sockaddr *from) {
	int32_t	 sock;
	STRING	 ptr, udpdata;
	UDP_HDR  *udphdr;

	sock = soc & SOCK_MASK;
	if(!(soc & UDP_SOCK) || sock >= UDPNUM) return -1;
	if(udp_table[sock].packet == 0) return -1;
	ptr = udp_table[sock].packet;
	udphdr = (UDP_HDR*)ptr;
	udpdata = &(ptr[sizeof(UDP_HDR)]);
	memcpy(data, udpdata, udphdr->data_size - sizeof(UDP_HDR));
	free_mem(meminfo, udp_table[sock].id, udp_table[sock].packet);
	udp_table[sock].packet = 0;
	from->sin_addr = udp_table[sock].src_ip;
	from->sin_port = udphdr->src_port;
	return udphdr->data_size - sizeof(UDP_HDR);
}

int32_t udp(uint32_t fd, IP_HDR *iphdr) {
	Task	*taskptr;
	UDP_HDR  *udphdr;
	Ether	 *ether_buf;
	STRING	 ptr, udpdata;
	int32_t	 datasize, sock, size;
	uint32_t dest_ip;
	uint16_t dest_port;
	struct sockaddr *from;

	ptr = (STRING)iphdr;
	ptr = &(ptr[iphdr->header_len * 4]);
	udphdr = (UDP_HDR*)ptr;
	datasize = iphdr->datasize;
	datasize -= (int32_t)iphdr->header_len * 4;
	datasize -= sizeof(UDP_HDR);
	udpdata =  &(ptr[sizeof(UDP_HDR)]);
	dest_ip = iphdr->destip;
	dest_port = udphdr->dest_port;
	for(sock = 0;sock < UDPNUM;sock++) {
		if(udp_table[sock].sin_port == dest_port &&
		   udp_table[sock].sin_addr == dest_ip) break;
	}
	if(sock == UDPNUM) return -1;
	taskptr = get_taskptr(udp_table[sock].id);
	if(taskptr != 0) {
	   	if(taskptr->req == UDP_RECV_REQ &&
		   sock == (taskptr->arg[0] & SOCK_MASK) &&
		   taskptr->net_count > 0) {
			size = datasize;
			if(size > taskptr->arg[2]) size = taskptr->arg[2];
			memcpy((STRING)(taskptr->arg[1]), udpdata, size);
			from = (struct sockaddr *)taskptr->arg[3];
			from->sin_addr = iphdr->srcip;
			from->sin_port = udphdr->src_port;
			taskptr->net_count = 0;
			taskptr->retval = size;
		} else {
			if(udp_table[sock].packet) {
				free_mem(meminfo, udp_table[sock].id, udp_table[sock].packet);
			}
			udp_table[sock].packet = alloc_mem(meminfo, udp_table[sock].id, udphdr->data_size);
			if(udp_table[sock].packet) {
				memcpy((STRING)(udp_table[sock].packet), (STRING)udphdr, udphdr->data_size);
				udp_table[sock].src_ip = iphdr->srcip;
			}
		}
		return sock;
	}
	return -1;
}
