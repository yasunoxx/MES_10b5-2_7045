/*
	2005/02/09
		A.Takeuchi modified.
*/
#include "../sys.h"

typedef struct {
	unsigned char  op	__attribute__((packed));
	unsigned char  htype	__attribute__((packed));
	unsigned char  hlen	__attribute__((packed));
	unsigned char  hops	__attribute__((packed));
	unsigned int xid	__attribute__((packed));
	unsigned short secs	__attribute__((packed));
	unsigned short flags	__attribute__((packed));
	unsigned int ciaddr	__attribute__((packed));
	unsigned int yiaddr	__attribute__((packed));
	unsigned int siaddr	__attribute__((packed));
	unsigned int giaddr	__attribute__((packed));
	unsigned char	 chaddr[16];
	unsigned char	 sname[64];
	unsigned char	 filename[128];
	unsigned char	 vinfo[64];
} DHCP;
#define DHCP_DISCOVER	1
#define DHCP_OFFER	2
#define DHCP_REQUEST	3
#define DHCP_PACK	5
#define DHCP_PNAK	6
#define DHCP_RELEASE	7

#define BOOT_REQUEST	1
#define BOOT_REPLY	2

static int rand(char* mymac){
	int	ret;

	// AT start
	ret=(mymac[3]<<16)|(mymac[4]<<8)|(mymac[5]);
	if(ret==0) ret=0x123456;
// there was a case that ret was always 100000000 in case mymac[5]==9
/*
	ret = 0;
	do {
		ret = 100000000/(((int)(mymac[5]+1)*12359763)/100000000);
	}while(ret >= 100000000);
*/
	// AT end
	return ret;
}

static void dhcp_packet(DHCP *data, unsigned int xid, char* mymac, short flag, unsigned int yiip, char msg_type) {
	data->op = BOOT_REQUEST;
	data->htype = 1;
	data->hlen = 6;
	data->hops = 0;
	data->xid = xid;
	data->secs = 0;
	data->flags = flag;
	data->ciaddr = 0;
	data->yiaddr = yiip;
	data->siaddr = 0;
	data->giaddr = 0;
	bzero(data->chaddr, 16);
	memcpy(data->chaddr, mymac, 6);
	bzero(data->sname, 64);
	bzero(data->filename, 127);
	bzero(data->vinfo, 64);
	data->vinfo[0] = 0x63;
	data->vinfo[1] = 0x82;
	data->vinfo[2] = 0x53;
	data->vinfo[3] = 0x63;
	data->vinfo[4] = 0x35;
	data->vinfo[5] = 0x01;
	data->vinfo[6] = msg_type;
	data->vinfo[7] = 0x3d;
	data->vinfo[8] = 0x07;
	data->vinfo[9] = 0x01;
	memcpy(&data->vinfo[10], mymac, 6);
}

static unsigned int dhcp_getip(char* mymac, unsigned int *servip,unsigned int *gwip, unsigned int *mask) {
	int		i, sock1, sock2;
	unsigned int	xid, ip, ret;
	DHCP		dhcpbuf, s_dhcpbuf;
	struct sockaddr	myaddr, addr, from;
	int		n;	// AT
	unsigned char	*p;	// AT

	ret=0;	// AT
	xid = rand(mymac);
	dhcp_packet(&dhcpbuf, xid, mymac, 0x8000, 0, DHCP_DISCOVER);
	dhcpbuf.vinfo[16] = 0xff;
	sock1 = udp_socket();
	myaddr.sin_addr = 0;
	myaddr.sin_port = 68;
	udp_bind(sock1, &myaddr);
	addr.sin_addr = IPADDR(255, 255, 255, 255);
	addr.sin_port = 67;
	sock2 = udp_socket();
	myaddr.sin_addr = IPADDR(255, 255, 255, 255);
	myaddr.sin_port = 68;
	udp_bind(sock2, &myaddr);
	sendto(sock1, (char*)&dhcpbuf, 300, &addr);
	n=0;	// not to have blocked AT
	do{
		if(recvfrom(sock2, (char*)&s_dhcpbuf, 300, &from)<=0)
		{ if(n++>2) goto skip;
	          sendto(sock1, (char*)&dhcpbuf, 300, &addr);
                }
	} while(s_dhcpbuf.xid != xid || memcmp(s_dhcpbuf.chaddr, mymac, 6) != 0);
	ip = s_dhcpbuf.yiaddr;
	// AT start
	if(s_dhcpbuf.giaddr) *gwip=s_dhcpbuf.giaddr;
	p=&(s_dhcpbuf.vinfo[4]);
	for(;;){
		if(p[0]==0xff) break;
		else if(p[0]==1 && p[1]==4) // netmask
			*mask=(p[2]<<24)|(p[3]<<16)|(p[4]<<8)|p[5];
		else if(p[0]==3 && p[1]>=4 && *gwip!=0) // routers
			*gwip=(p[2]<<24)|(p[3]<<16)|(p[4]<<8)|p[5];
		p=p+p[1]+2;
	}
	xid|=0x1000000;   // to avoid to mistake OFFER packet
	dhcp_packet(&dhcpbuf, xid, mymac, 0x8000, ip, DHCP_REQUEST);
        p=&(dhcpbuf.vinfo[16]);
	*p++ = 0x32;
	*p++ = 0x04;
	*p++ = (ip >> 24);
	*p++ = (ip >> 16);
	*p++ = (ip >> 8);
	*p++ = ip;
        *p++ = 0x36;
        *p++ = 0x04;
        *p++ = (from.sin_addr >> 24);
        *p++ = (from.sin_addr >> 16);
        *p++ = (from.sin_addr >> 8);
        *p++ = from.sin_addr;
        *p   = 0xff;
	// AT end
	sendto(sock1, (char*)&dhcpbuf, 300, &addr);
	n=0;	// not to have blocked AT
	do{
		if(recvfrom(sock2, (char*)&s_dhcpbuf, 300, &from)<=0)
		{ if(n++>2) goto skip;
	          sendto(sock1, (char*)&dhcpbuf, 300, &addr);
                }
	} while(s_dhcpbuf.xid != xid || memcmp(s_dhcpbuf.chaddr, mymac, 6) != 0);
	*servip = from.sin_addr;
	// AT start
	p=&(s_dhcpbuf.vinfo[4]);
	for(;;){
		if(p[0]==0xff) break;
		else if(p[0]==0x35 && p[1]==1 && p[2]==DHCP_PACK) {
			ret=ip;break;
		}
		p=p+p[1]+2;
	}
	// AT end
/*
	if(s_dhcpbuf.vinfo[6] == DHCP_PACK) ret = ip;
	else ret = 0;
*/
skip:;
	udp_free(sock2);
	udp_free(sock1);
	return ret;
}

int if_dhcp(char *dev)
{
	unsigned int ret, ip, mask, gwip, servip;
	char	 mymac[6];

	ret = ifconfig(dev, 0, 0);
	if(ret != 0) return 0;
//	get_mac_address(0, mymac);
	ret = get_ipinfo(0,&ip,&mask,mymac);	// AT
	if(ret == -1) return 0;		// AT
	gwip=mask=0;
	ip = dhcp_getip(mymac, &servip, &gwip, &mask);
	if(ip == 0) return 0;
	// AT start
	if(gwip==0) gwip=servip; // if DHCP didn't provide gatewayip, servip is substitute
	if(mask==0) {
	  if((ip&0xffffff00)==(gwip&0xffffff00)) mask=0xffffff00;
	  else if((ip&0xffff0000)==(gwip&0xffff0000)) mask=0xffff0000;
	  else if((ip&0xff000000)==(gwip&0xff000000)) mask=0xff000000;
        }
	ifconfig(dev, ip, mask);
	ifgateway(gwip);
	// AT end
	return servip;
}
