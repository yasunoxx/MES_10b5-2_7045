/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/*  Modified By				*/
/*     T.Mochizuki 04/03/09             */
/*  Modified By                         */
/*     A.Takeuchi  04/10/05             */
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "ip.h"
#define SuperPID 1
#define MAX_IP_TABLE 30
#define ARP_LIVETIME    1000

extern MemInfo	meminfo;
int32_t ether_header(uint32_t, Ether*, uint32_t, uint32_t);
int32_t set_ip_addr(int32_t, uint32_t, Byte*);

uint32_t gateway_ip;
static const Byte bcast_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
static const Byte zero_mac[6] = {0,0,0,0,0,0};
IP_MAC   *mac_Table;



static int32_t send_arp_packet(int32_t fd, uint16_t op, uint32_t req_ip) {
	int32_t	 index, ret;
	STRING	 buffer;
	Byte	 mac[6];
	uint32_t ip, mask;
	Ether	 *ether_buf;
	Arp	 *arp_buf;

	index = fd2index(fd);
	if(index == -1) return -1;
	ret = get_ipinfo(index, &ip, &mask, mac);
	if(ret == -1) return -1;

	buffer = alloc_mem(meminfo, SuperPID, sizeof(Ether) + sizeof(Arp));
	if(buffer == 0) return -1;
	ether_buf = (Ether*)buffer;
	arp_buf = (Arp*)&ether_buf[1]; // Add T.Mochizuki
	switch(op) {
	case ARP_REQ:
		memcpy(ether_buf->src, mac, 6);
		memcpy(ether_buf->dest, bcast_mac, 6);
		ether_buf->type = ARP_TYPE;
		bzero(arp_buf->destmac, 6); 
		break;
	case ARP_ANS:
		ether_header(fd, ether_buf, req_ip, ARP_TYPE);
		memcpy(arp_buf->destmac, ether_buf->dest, 6);// Mod T.Mochizuki
		break;
	}
	arp_buf->hard = ETHER_TYPE;
	arp_buf->proto = IP_TYPE;
	arp_buf->hardsize = 6;
	arp_buf->protosize = 4;
	arp_buf->op = op;
	memcpy(arp_buf->srcmac, mac, 6);
	arp_buf->srcip = ip;
	arp_buf->destip = req_ip;
	write_device(fd, buffer, sizeof(Ether) + sizeof(Arp));
	free_mem(meminfo, SuperPID, buffer);
	return 0;
}

int32_t select_net(uint32_t req_ip) {
	int32_t	 index, ret;
	Byte	 mac[6];
	uint32_t ip, mask;

	for(index = 0;index < IPNUM;index++) {
		ret = get_ipinfo(index, &ip, &mask, mac);
		if(ret == -1) continue;
		if((req_ip & mask) == (ip & mask)) return index;	
	}
	return -1;
}

int32_t if_gateway(uint32_t ip) {
	uint32_t ret;
//	ret = (ip == 0) ? gateway_ip : (gateway_ip = ip);
	ret = (ip == 0xffffffff) ? gateway_ip : (gateway_ip = ip);	// AT
	return ret;
}

void set_mac(uint32_t ip, Byte *mac) 
{
  IP_MAC  *mac_tbl;
  int32_t mn;
  int16_t i,j,j2;

  if ( mac_Table == 0 ) return;
  if ( select_net(ip) == -1 ) return;
  mn = ARP_LIVETIME;
  j = j2 = -1;
  mac_tbl = mac_Table;
  for ( i = 0 ; i < MAX_IP_TABLE; i++ ) {
    if ( mac_tbl->timeout > 0 ) {
      if ( mac_tbl->ip == ip ) {
	j = i;
	break;
      }
      if ( mn > mac_tbl->timeout ) {
	mn = mac_tbl->timeout;
	j = i;
      }
    } else {
      j2 = i;
    }
    mac_tbl++;
  }
  if ( i == MAX_IP_TABLE && j2 >= 0 ) j = j2;
  mac_Table[j].ip = ip;
  mac_Table[j].timeout = ARP_LIVETIME;
  memcpy(mac_Table[j].mac,mac,6);
  return;
}

int32_t arp(uint32_t fd, Arp *arp) {
	int32_t	 ret,index,ret_getip;
	Byte	 mac[6];
	uint32_t ip, mask;

	if(arp->proto != IP_TYPE) return -1;
	if(arp->hardsize != 6) return -1;
	if(arp->protosize != 4) return -1;
	set_mac(arp->srcip, arp->srcmac); // Add T.Mochizuki
	//push_table(arp->srcip, arp->srcmac);
	ret = 0;
	switch(arp->op) {
	case ARP_REQ:
	  // Mod T.Mochizuki
	        index = fd2index(fd);
	        if(index == -1) break;
	        ret_getip = get_ipinfo(index, &ip, &mask, mac);
	        if(ret_getip == -1) break;
		if ( ip != arp->destip ) break;
		ret = send_arp_packet(fd, ARP_ANS, arp->srcip);
		break;
	case ARP_ANS:
		ret = 0;
		break;
	default:
		ret = -1;
	}
	return ret;
}

int32_t send_arp(uint32_t _req_ip) {
	int32_t	 ret, fd, index, req_ip;

	req_ip = _req_ip;
	index = select_net(req_ip);
	if(index == -1) {
	  if(gateway_ip == 0) return -1;
	  req_ip = gateway_ip;
	  index = select_net(req_ip);
	  if(index == -1) return -1;
	}
	fd = index2fd(index);
	if(fd == -1) return -1;
	ret = send_arp_packet(fd, ARP_REQ, req_ip);
	return ret;
}
/* Found 	: 1
   Not found	: -1
   NO NIC	: 0 */
int32_t get_mac(uint32_t *ip, Byte *mac) 
{
  IP_MAC  *mac_tbl;
  int16_t i,j;
  if ( mac_Table == 0 ) return 0;
  mac_tbl = mac_Table;
  if ( select_net(*ip) == -1 ) *ip = gateway_ip;
  if((*ip)==0xffffffff)
  { memcpy(mac,bcast_mac,6);  //  AT in case of broadcast IP
    return 1;
  }
  else
  { for ( i = 0 ; i < MAX_IP_TABLE; i++ ) {
      if ( mac_tbl->timeout > 0 && mac_tbl->ip == (*ip) ) {
        j = i;
        break;
      }
      mac_tbl++;
    }
    if ( i != MAX_IP_TABLE )  {
      memcpy(mac,mac_Table[j].mac,6);
      return 1;
    }
  }
  return -1;
}
void gc_mac()
{
  IP_MAC  *mac_tbl;
  int16_t i;
  if ( mac_Table == 0 ) return;
  mac_tbl = mac_Table;
  for ( i = 0 ; i < MAX_IP_TABLE; i++ ) {
    if ( mac_tbl->timeout > 0 ) mac_tbl->timeout--;
    mac_tbl++;
  }
}
void arp_init() {
	gateway_ip = 0;
	mac_Table = (IP_MAC*)alloc_mem(meminfo, SuperPID, sizeof(IP_MAC) * MAX_IP_TABLE);
	if ( mac_Table > 0 ) memset(mac_Table,0,sizeof(IP_MAC) * MAX_IP_TABLE);
	else mac_Table = 0;
}
