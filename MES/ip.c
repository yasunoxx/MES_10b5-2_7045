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
#include "ip.h"

#define SuperPID	1
#define	MAXPKT		256
#define IP_FLAG		1514
#define IP_ENABLE	1
#define ARP_CNTDOWN_COUNT 400
typedef struct {
	uint32_t fd;
	uint32_t ip, mask;
	uint16_t ip_id;
	Byte	 mac[6], name[7], flag;
} NetInfo;

typedef struct {
	STRING	data;
	int32_t size, fd;
        int32_t count;
} RcvInfo;

struct _flag_info{
	STRING		  data;
	struct _flag_info *next;
};
typedef struct _flag_info FlagInfo;

typedef struct {
	FlagInfo *data;
	uint16_t identifer;
	int32_t	 time;
} RootFlag;

#define MAX_FLAG 128
#define FLAGNUM	 6

static NetInfo  netinfo[IPNUM];
static QueInfo	rcv_fifo[2];
static FlagInfo flaginfo[MAX_FLAG];
static RootFlag	flagroot[FLAGNUM];
static int8_t	rcv_index;
static int32_t  arp_cnt = 0;
extern MemInfo	meminfo, tmpmem;
extern int32_t	stdio, sys_count;
int32_t arp(uint32_t, Arp*);
int32_t get_mac(uint32_t*, Byte*);
void    gc_mac();
void    set_mac(uint32_t,Byte*);
int32_t get_ipinfo(int32_t, uint32_t*, uint32_t*, Byte*);
int32_t select_net(uint32_t);

uint16_t make_crc(STRING data, int32_t size) {
	uint32_t	i, a, b;

	a = 0;
	for(i = 0;i < size;i += 2) {
		b = ((uint32_t)data[i] & 0xff) * 256;
		if((i + 1) < size) b |= (uint32_t)data[i + 1] & 0xff;
		a = a + b;
		if(a & 0x10000) a++;
		a &= 0xffff;
	}
	return (uint16_t)(~a & 0xffff);
}

int32_t fd2index(int32_t fd) {
	int32_t	index;

	for(index = 0;index < IPNUM;index++) {
		if(netinfo[index].fd == fd) return index;
	}
	return -1;
}

int32_t index2fd(int32_t index) {
	int32_t	fd;

	if(netinfo[index].fd == 0) return -1;
	return netinfo[index].fd;
}

int32_t set_ip_addr(int32_t index, uint32_t destip, Byte *destmac) {
	uint32_t ip, mask, ret;
	Byte	 mac[6];

	ret = get_ipinfo(index, &ip, &mask, mac);
	if(ret == -1) return -1;
//	if((netinfo[index].flag & IP_ENABLE) == 0) {
	if(ip!=0 || (netinfo[index].flag & IP_ENABLE) == 0) {  // AT
		if(memcmp(mac, destmac, 6) != 0) return -1;
		netinfo[index].ip = destip;
                if(mask==0)		// AT
		{ netinfo[index].mask = IPADDR(255,255,255,0);
		}
		netinfo[index].flag |= IP_ENABLE;
	}
	return 0;
}

int32_t tcpip(uint32_t fd, IP_HDR *iphdr, Byte *destmac) {
	int32_t	 index, ret;
	uint32_t ip, mask;
	Byte	 mac[6];

	if(iphdr->version != 4)  return -1;
	index = fd2index(fd);
	if(index == -1) return -1;
	ret = set_ip_addr(index, iphdr->destip, destmac);
	if(ret == -1) return -1;
	ret = get_ipinfo(index, &ip, &mask, mac);
	if(ret == -1) return -1;
	if(ip != iphdr->destip && (iphdr->destip | mask) != 0xffffffff) return -1;
	switch(iphdr->proto) {
	case ICMP:
		icmp(fd, iphdr);
		break;
	case UDP:
		udp(fd, iphdr);
		break;
	case TCP:
		tcp(fd, iphdr);
		break;
	}
	return 0;
}

static void receive(uint32_t fd) {
	STRING	packet, data;
	int32_t c, size, max;
	RcvInfo	rcv_info;

	max = ioctl_device(fd, 0, MAXPACKET);
	packet = alloc_mem(tmpmem, SuperPID, max);
	if(packet == 0) return;
	rcv_info.fd = fd;
	while((size = read_device(fd, packet, max)) > 0) {
		data = alloc_mem(meminfo, SuperPID, size + sizeof(STRING));
		if(data != 0) {
			rcv_info.data = data;
			rcv_info.size = size;
			rcv_info.count = 0;
			bzero(data, sizeof(STRING));
			data = &(data[sizeof(STRING)]);
			memcpy(data, packet, size);
			push_fifo(&rcv_fifo[rcv_index], &rcv_info);
		}
	}
	free_mem(tmpmem, SuperPID, packet);
}

void	free_flag_data(int32_t index) {
	FlagInfo	*ptr;

	for(ptr = flagroot[index].data;ptr->next != 0;ptr = ptr->next) {
		free_mem(meminfo, SuperPID, (STRING)ptr->data);
		ptr->data = 0;
	}
	free_mem(meminfo, SuperPID, (STRING)ptr->data);
	ptr->data = 0;
	flagroot[index].data = 0;
	flagroot[index].identifer = 0;
	flagroot[index].time = 0;
}

int32_t check_flagroot_index() {
	int32_t diff_time, index;

	for(index = 0;index < FLAGNUM;index++) {
		if(flagroot[index].data != 0) {
			diff_time = sys_count - flagroot[index].time;
			if(diff_time > 12) free_flag_data(index);
		}
	}	
}

int32_t get_flagroot_index() {
	int32_t diff_time, old_time, index, index2;

	for(index = 0;index < FLAGNUM;index++) {
		if(flagroot[index].data == 0) {
			flagroot[index].time = sys_count;
			flagroot[index].identifer = 0;
			return index;
		}
	}
	for(index = 0;index < FLAGNUM;index++) {
		diff_time = sys_count - flagroot[index].time;
		if(diff_time > 12) {
			free_flag_data(index);
		}
	}	
	for(index = 0;index < FLAGNUM;index++) {
		if(flagroot[index].data == 0) {
			flagroot[index].time = sys_count;
			flagroot[index].identifer = 0;
			return index;
		}
	}
	old_time = sys_count;
	index2 = 0;
	for(index = 0;index < FLAGNUM;index++) {
		if(flagroot[index].time < old_time) {
			old_time = flagroot[index].time;
			index2 = index;
		}
	}
	free_flag_data(index2);
	flagroot[index2].data = 0;
	flagroot[index].identifer = 0;
	flagroot[index2].time = sys_count;
	return index2;
}

int32_t get_flag_index() {
	int32_t	index;

	for(index = 0;index < MAX_FLAG;index++) {
		if(flaginfo[index].data == 0) return index;
	}
	return -1;
}

int32_t ip_flag_save(STRING data) {
	Ether		*ether, *ether2;
	IP_HDR		*ip_ptr, *ip_ptr2;
	int32_t		index, i;
	FlagInfo	*ptr;

	ether = (Ether*)&data[sizeof(STRING)];
	ip_ptr = (IP_HDR*)&ether[1];
	i = get_flag_index();
	if(i == -1) {
		free_mem(meminfo, SuperPID, data);
		return -1;
	}
	for(index = 0;index < FLAGNUM;index++) {
		if(flagroot[index].identifer == ip_ptr->identifer) {
			ptr = flagroot[index].data;
			ether2 = (Ether*)&((ptr->data)[sizeof(STRING)]);
			ip_ptr2 = (IP_HDR*)&ether2[1];
			if(ip_ptr->offset < ip_ptr2->offset){
				flaginfo[i].data = data;
				flaginfo[i].next = flagroot[index].data;
				flagroot[index].data = &(flaginfo[i]);
				return 0;
			}
			for(;ptr->next != 0;ptr = ptr->next) {
				ether2 = (Ether*)&((ptr->data)[sizeof(STRING)]);
				ip_ptr2 = (IP_HDR*)&ether2[1];
				if(ip_ptr->offset < ip_ptr2->offset) break;
			}
			flaginfo[i].data = data;
			flaginfo[i].next = ptr->next;
			ptr->next = &(flaginfo[i]);
			return 0;
		}
	}
	index = get_flagroot_index();
	flaginfo[i].data = data;
	flaginfo[i].next = 0;
	flagroot[index].identifer = ip_ptr->identifer;
	flagroot[index].data = &(flaginfo[i]);
	return 0;
}

int32_t check_flag(STRING data, int32_t fd) {
	Ether		*ether, *ether2;
	IP_HDR		*ip_ptr, *ip_ptr2, *ip;
	int32_t		index, i, hdrsiz, offset, offset2;
	int32_t		flag, datsiz, op, ipsiz;
	uint16_t	crc;
	FlagInfo	*ptr;
	STRING		src, dest;

	ether = (Ether*)&data[sizeof(STRING)];
	ip_ptr = (IP_HDR*)&ether[1];
	hdrsiz = ip_ptr->header_len * 4;
	ipsiz = flag = offset = 0;
	for(op = 0;op < 2;op++) {
		for(index = 0;index < FLAGNUM;index++) {
			if(flagroot[index].identifer == ip_ptr->identifer) {
				if(op == 1) {
					ip = (IP_HDR*)alloc_mem(tmpmem, SuperPID, ipsiz);
					if(ip == 0) {
						free_flag_data(index);
						return -1;
					}
					memcpy((STRING)ip, (STRING)ip_ptr, hdrsiz);
					ip->datasize = ipsiz;
					ip->identifer = 0;
					ip->end_packet = 0;
					ip->offset = 0;
					ip->checksum = 0;
					crc = make_crc((STRING)ip, hdrsiz);
					ip->checksum = crc;
					dest = (STRING)ip;
					dest = &dest[hdrsiz];
				}
				for(ptr = flagroot[index].data;ptr->next != 0;ptr = ptr->next) {
					ether2 = (Ether*)&((ptr->data)[sizeof(STRING)]);
					ip_ptr2 = (IP_HDR*)&ether2[1];
					datsiz = ip_ptr2->datasize - hdrsiz;
					offset2 = ip_ptr2->offset * 8;
					if(op == 0) if(offset != offset2) flag = 1;
					if(op == 1) {
						src = (STRING)ip_ptr2;
						src = &src[hdrsiz];
						memcpy(dest, src, datsiz);
						dest = &dest[datsiz];
					}
					offset += datsiz;
				}
				ether2 = (Ether*)&((ptr->data)[sizeof(STRING)]);
				ip_ptr2 = (IP_HDR*)&ether2[1];
				datsiz = ip_ptr2->datasize - hdrsiz;
				offset2 = ip_ptr2->offset * 8;
				if(op == 0) {
					if(offset != offset2) flag = 1;
					if(ip_ptr2->end_packet) flag = 1;
					if(flag == 0) ipsiz = offset + datsiz + hdrsiz;
					else return -1;
				}
				if(op == 1) {
					src = (STRING)ip_ptr2;
					src = &src[hdrsiz];
					memcpy(dest, src, datsiz);
					tcpip(fd, ip, ether->dest);
					free_flag_data(index);
					free_mem(tmpmem, SuperPID, (STRING)ip);
				}
				offset += datsiz;
			}
		}
	}
	return -1;
}

void ip_request() {
	RcvInfo	rcv_info;
	int32_t	c, n, fd, ret;
	uint32_t ip;
	Ether	*ether;
	Arp	*arp_ptr;
	IP_HDR	*ip_ptr;
	Byte mac[6];

	rcv_index = (rcv_index == 0) ? 1 : 0; 
	n = que_size(&rcv_fifo[rcv_index]);
	for(c = 0;c < n;c++) {
		pop_fifo(&rcv_fifo[rcv_index], &rcv_info);
		fd = rcv_info.fd;
		ether = (Ether*)&rcv_info.data[sizeof(STRING)];
		switch(ether->type) {
		case ARP_TYPE:
			arp_ptr = (Arp*)&ether[1];
			arp(fd, arp_ptr);
			free_mem(meminfo, SuperPID, rcv_info.data);
			break;
		case IP_TYPE:
			ip_ptr = (IP_HDR*)&ether[1];
			ip = ip_ptr->srcip;
			if ( select_net(ip) == -1 ) {
			  if ( get_mac(&ip,mac) == -1 ) {
			    if ( rcv_info.count == 0 ) {
			      send_arp(ip);
			      rcv_info.count = 2000;
			      push_fifo(&rcv_fifo[rcv_index], &rcv_info);
			    } else if ( rcv_info.count == 1 ) {
			      free_mem(meminfo, SuperPID, rcv_info.data);
			      rcv_info.count = 0;
			    } else {
			      rcv_info.count --;
			      push_fifo(&rcv_fifo[rcv_index], &rcv_info);
			    }
			    break;
			  }
			} else {
			  set_mac(ip_ptr->srcip, ether->src); 
			}
			if(ip_ptr->end_packet == 0 && ip_ptr->offset == 0) {
				tcpip(fd, ip_ptr, ether->dest);
				free_mem(meminfo, SuperPID, rcv_info.data);
			} else {
				ip_flag_save(rcv_info.data);
				ret = check_flag(rcv_info.data, fd);
			}
			break;
		default:
			free_mem(meminfo, SuperPID, rcv_info.data);
			break;
		}
	}
	if ( arp_cnt < 1 ) {
	  arp_cnt = ARP_CNTDOWN_COUNT;
	  gc_mac();
	}
	arp_cnt--;
}

int32_t if_config2(STRING name, uint32_t ip, uint32_t mask) {
	int32_t	index, fd;

	for(index = 0;index < IPNUM;index++) {
		if(netinfo[index].fd == 0) {
			fd = open_device(SuperPID, name, 0);
			if(fd == -1) return -1;
			netinfo[index].fd = fd;
			ioctl_device(netinfo[index].fd, (int)&receive, SET_HANDLE);
			strncpy(netinfo[index].name, name, 7);
		}
		if(netinfo[index].fd != 0 && strncmp(netinfo[index].name, name, 7) == 0) {
			netinfo[index].ip = ip;
			netinfo[index].mask = mask;
			netinfo[index].flag = IP_ENABLE;
			ioctl_device(netinfo[index].fd, (int)(netinfo[index].mac), GET_MAC);
			return index;
		}
	}
	return -1;
}

int32_t if_config(STRING name, uint32_t ip, uint32_t mask) {
	return if_config2(name, ip, mask);
}

int32_t if_up(STRING name) {
	int32_t	index;
	index = if_config2(name, 0, 0);
	if(index != -1) netinfo[index].flag = 0;
	return index;
}

int32_t if_down(STRING name) {
	int32_t	index, fd;

	for(index = 0;index < IPNUM;index++) {
		if(netinfo[index].fd == 0) continue;
		if(netinfo[index].fd != 0 && strncmp(netinfo[index].name, name, 7) == 0) {
			close_device(SuperPID, netinfo[index].fd);
			netinfo[index].fd = 0;
			netinfo[index].flag = 0;
			return 0;
		}
	}
	return -1;
}

int32_t get_ipinfo(int32_t index, uint32_t *ip, uint32_t *mask, Byte *mac) {
	if(index < 0 || index >= IPNUM) return -1;
	if(netinfo[index].fd == 0) return -1;
	*ip = netinfo[index].ip;
	*mask = netinfo[index].mask;
	memcpy(mac, netinfo[index].mac, 6);
	return 0;
}

uint32_t get_ip(int32_t index) {
	if(index < 0 || index >= IPNUM) return -1;
	if(netinfo[index].fd == 0) return -1;
	return netinfo[index].ip;
}

int32_t ether_header(uint32_t fd, Ether *ether, uint32_t ip, uint32_t protocol) {
	int32_t	index;
//	int32_t iflg;// Add Mochizuki

	index = fd2index(fd);
	if(index == -1) return -1;
	if(get_mac(&ip, ether->dest) != 1) return -1;
	memcpy(ether->src, netinfo[index].mac, 6);
	ether->type = protocol;
	return 0;
}

void ip_opt_header(Ether *ether, STRING opt, uint32_t size) {
	IP_HDR	 *ip_ptr;
	int32_t	 value;

	ip_ptr = (IP_HDR*)&ether[1];
	value = size + sizeof(IP_HDR);
	ip_ptr->datasize = value;
	value >>= 2;
	ip_ptr->header_len = value;
	if(size > 0) memcpy((STRING)&ip_ptr[1], opt, size);
}

int32_t ip_header(uint32_t fd, Ether *ether, uint32_t ip, uint32_t protocol, uint32_t size) {
	int32_t	 index, header_size;
	uint16_t crc;
	IP_HDR	 *ip_ptr;
	uint32_t ip2;
	ip2 = ip;
	index = fd2index(fd);
	if(index == -1) return -1;
	if(get_mac(&ip2, ether->dest) == -1) return -1;
	ether_header(fd, ether, ip, IP_TYPE);
	ip_ptr = (IP_HDR*)&ether[1];
	ip_ptr->version = 4;
	ip_ptr->service = 0;
	header_size = ip_ptr->datasize;
	ip_ptr->datasize += size;
	ip_ptr->identifer = (netinfo[index].ip_id)++;
	ip_ptr->div_enable = 1;
	ip_ptr->end_packet = 0;
	ip_ptr->offset = 0;
	ip_ptr->ttl = 0x40;
	ip_ptr->proto = protocol;
	ip_ptr->checksum = 0;
	ip_ptr->srcip = netinfo[index].ip;
	ip_ptr->destip = ip;
	crc = make_crc((STRING)ip_ptr, header_size);
	ip_ptr->checksum = crc;
	return header_size + sizeof(Ether);
}

int32_t write_ip_packet(int32_t fd, Ether *ether, int32_t size) {
	static	int16_t	counter = 1;
	STRING	packet, dest, src;
	IP_HDR	*ip_ptr, *ip_ptr2;
	Ether	*ether2;
	int32_t	hdrsiz, remain, sendsize, offset, flagsize;
	uint16_t crc;

	ip_ptr = (IP_HDR*)&ether[1];
	packet = alloc_mem(tmpmem, SuperPID, IP_FLAG);
	if(packet == 0) return -1;
	ether2 = (Ether*)packet;
	ip_ptr2 = (IP_HDR*)&ether2[1];
	hdrsiz = ip_ptr->header_len * 4;
	flagsize = IP_FLAG - sizeof(Ether) - hdrsiz;
	remain = ip_ptr->datasize - hdrsiz;
	offset = 0;
	memcpy(packet, (STRING)ether, sizeof(Ether) + hdrsiz);
	dest = &packet[sizeof(Ether) + hdrsiz];
	src = (STRING)ether;
	src = &src[sizeof(Ether) + hdrsiz];
	ip_ptr2->identifer = counter++;
	if(remain <= flagsize) ip_ptr2->identifer = 0;
	while(remain > 0) {
		sendsize = (remain > flagsize) ? flagsize : remain;
		remain -= sendsize;
		ip_ptr2->datasize = sendsize + hdrsiz;
		ip_ptr2->div_enable = 1;
		ip_ptr2->end_packet = (remain == 0) ? 0 : 1;
		ip_ptr2->offset = offset / 8;
		ip_ptr2->checksum = 0;
		crc = make_crc((STRING)ip_ptr2, hdrsiz);
		ip_ptr2->checksum = crc;
		memcpy(dest, src, sendsize);
		src = &src[sendsize];
		write_device(fd, packet, sizeof(Ether) + hdrsiz + sendsize);
		offset += sendsize;
	}
	free_mem(tmpmem, SuperPID, packet);
	return size;
}

void ip_init() {
	int32_t	c, index;
	STRING	ptr;

	for(index = 0;index < IPNUM;index++) {
		netinfo[index].fd = 0;
		netinfo[index].ip_id = 1;
		netinfo[index].flag = 0;
	}
	for(index = 0;index < MAX_FLAG;index++) {
		flaginfo[index].data = 0;
		flaginfo[index].next = 0;
	}
	for(index = 0;index < FLAGNUM;index++) {
		flagroot[index].data = 0;
		flagroot[index].time = 0;
		flagroot[index].identifer = 0;
	}
	arp_init();
	udp_init();
	tcp_init();
	for(c = 0;c < 2;c++) {
		ptr = alloc_mem(meminfo, SuperPID, sizeof(RcvInfo) * MAXPKT);
		if(ptr > 0) {
			bzero(ptr, sizeof(RcvInfo) * MAXPKT);
			init_que(&rcv_fifo[c], sizeof(RcvInfo), MAXPKT, ptr);
		} else {
			init_que(&rcv_fifo[c], sizeof(RcvInfo), 0, 0);
		}
	}
	rcv_index = 0;
}
