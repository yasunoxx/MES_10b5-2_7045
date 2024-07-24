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
#define SuperPID 1

extern MemInfo	tmpmem, meminfo;
extern int32_t	stdio;
void ip_opt_header(Ether*, STRING, uint32_t);
int32_t ip_header(uint32_t, Ether*, uint32_t, uint32_t, uint32_t);

void icmpreq_setdata()
{
}
void icmpreq_send()
{
}
int32_t icmp(uint32_t fd, IP_HDR *iphdr) {
	ICMP_HDR *icmphdr, *myicmp;
	Ether	 *ether_buf;
	STRING	 ptr;
	int32_t	 datasize, ret, size, stat;
	uint16_t crc;
	uint32_t dstip;

	ptr = (STRING)iphdr;
	ptr = &(ptr[iphdr->header_len * 4]);
	icmphdr = (ICMP_HDR*)ptr;
	datasize = iphdr->datasize;
	datasize -= (int32_t)iphdr->header_len * 4;
	ret = 0;
	switch(icmphdr->type) {
	case ICMP_ANS:
		break;
	case ICMP_REQ:
	  dstip = iphdr->srcip;
	  ptr = alloc_mem(tmpmem, SuperPID, sizeof(Ether) + sizeof(IP_HDR) + datasize);
	  if(ptr == 0) return -1;
	  bzero(ptr, sizeof(Ether) + sizeof(IP_HDR) + datasize);
	  ether_buf = (Ether*)ptr;
	  ip_opt_header(ether_buf, 0, 0);
	  size = ip_header(fd, ether_buf, iphdr->srcip, ICMP, datasize);
	  ptr = (STRING)&ether_buf[1];
	  ptr = &(ptr[sizeof(IP_HDR)]);
	  myicmp = (ICMP_HDR*)ptr;
	  memcpy((STRING)myicmp, (STRING)icmphdr, datasize);
	  size += datasize;
	  myicmp->type = ICMP_ANS;
	  myicmp->code = 0;
	  myicmp->checksum = 0;
	  crc = make_crc((STRING)myicmp, datasize);
	  myicmp->checksum = crc;
	  write_ip_packet(fd, (STRING)ether_buf, size);
	  free_mem(tmpmem, SuperPID, ptr);
	  break;
	default:
		ret = -1;
	}
	return ret;
}
