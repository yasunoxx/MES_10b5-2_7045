/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "const.h"
#include "pubtype.h"
#include "regdef.h"
#include "ne.h"

//#define NE_NUM		2
#define NE_NUM		1	// AT
static NEInfo ne[NE_NUM];
int32_t alloc_irq(int32_t, void (*)(int32_t), int32_t);
int32_t free_irq(int32_t);
void	set_bussize(int32_t, int32_t);

static void ne_outb(int32_t num, int32_t port, Byte data) {
	volatile Byte *reg = ne[num].base;

	reg[port] = data;
}

static Byte ne_inb(int32_t num, int32_t port) {
	volatile Byte *reg = ne[num].base;

	return reg[port];
}

static void ne_outw(int32_t num, int32_t port, uint16_t data) {
	uint32_t	value;

	value = data & 0xff;
	ne_outb(num, port, value);
	value = (data >> 8) & 0xff;
	ne_outb(num, port + 1, value);
}

static void ne_reset(int32_t num) {
	volatile int32_t	w;
	Byte			a;

	a = ne_inb(num, NE_ASIC_RESET);
	ne_outb(num, NE_ASIC_RESET, 0);
	for(w = 0;w < 0x200;w++);
	ne_outb(num, NE_P0_COMMAND, NE_CR_RD2 | NE_CR_STP);
	for(w = 0;w < 0x200;w++);
}

static int32_t ne_writemem(int32_t num, Byte *src, uint32_t dest, uint32_t size) {
	int32_t		c;

	if(size == 0) return 0;
	ne_outb(num, NE_P0_COMMAND, NE_CR_RD2 | NE_CR_STA);
	ne_outb(num, NE_P0_ISR, 0);
	ne_outw(num, NE_P0_RBCR0, size);
	ne_outw(num, NE_P0_RSAR0, dest);
	ne_outb(num, NE_P0_COMMAND, NE_CR_RD1 | NE_CR_STA);
	for(c = 0;c < size;c++) ne_outb(num, NE_ASIC_DATA, src[c]);
	c = 0;
	while(!(ne_inb(num, NE_P0_ISR) & NE_ISR_RDC)) {
		if(++c > 1000000) return -1;
	}
	return 0;
}

static void ne_readmem(int32_t num, uint32_t src, Byte *dest, uint32_t size) {
	int32_t		i;

	if(size == 0) return;
	ne_outb(num, NE_P0_COMMAND, NE_CR_RD2 | NE_CR_STA);
	ne_outw(num, NE_P0_RBCR0, size);
	ne_outw(num, NE_P0_RSAR0, src);
	ne_outb(num, NE_P0_COMMAND, NE_CR_RD0 | NE_CR_STA);
	for(i = 0;i < size;i++) dest[i] = ne_inb(num, NE_ASIC_DATA);
}

static void ne_mem_probe(int32_t num) {
	uint32_t	a, flag;
	Byte		buffer[20], message[20] = "NE2000 test message";

	ne[num].start = ne[num].size = 0;
	bzero(buffer, 20);
	for(a = 0x100;a < 0x10000;a += 0x100) {
		if(ne_writemem(num, buffer, a, 20) == -1) return;
	}
	flag = 0;
	for(a = 0x100;a < 0x10000;a += 0x100) {
		ne_readmem(num, a, buffer, 20);
		if(memcmp(buffer, message, 20) == 0) continue;
		ne_writemem(num, message, a, 20);
		ne_readmem(num, a, buffer, 20);
		if(memcmp(buffer, message, 20) == 0) {
			if(flag == 0) {
				flag = 1;
				ne[num].start = a;
				ne[num].size = 0x100;
			} else {
				ne[num].size += 0x100;
			}
		} else {
			flag = 0;
		}
	}
}

static void ne_stop(int32_t num) {
	ne_outb(num, NE_P0_COMMAND, NE_CR_STP);
	ne_outw(num, NE_P0_RBCR0, 0);
	ne_outb(num, NE_P0_RCR, NE_RCR_MON);
	ne_outb(num, NE_P0_TCR, NE_TCR_LB0);
}

static void ne_start(int32_t num) {
	ne_outb(num, NE_P0_COMMAND, NE_CR_STA);
	ne_outb(num, NE_P0_RCR, NE_RCR_AB);
	ne_outb(num, NE_P0_TCR, 0);
}

static int32_t ne_probe(int32_t num) {
	int32_t	i;
	Byte	eeprom[32], cmp, n;

	ne_reset(num);

	ne_outb(num, NE_P0_PSTART, 0x5a);
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS1 | NE_CR_STP);
	for(n = 1;n != 0;n <<= 1) {
		ne_outb(num, NE_P1_PAR0, n);
		cmp = ne_inb(num, NE_P1_PAR0);
		if(cmp != n) return -1;
	}
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS2 | NE_CR_STP);
	cmp = ne_inb(num, NE_P0_PSTART);
	if(cmp != 0x5a) return -1;
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS0 | NE_CR_STP);
	ne_outb(num, NE_P0_RCR, NE_RCR_MON);
	ne_outb(num, NE_P0_DCR, NE_DCR_BOS | NE_DCR_FT1 | NE_DCR_LS);
	ne_outb(num, NE_P0_TCR, NE_TCR_LB0);
	ne_mem_probe(num);
	ne_readmem(num, 0, eeprom, 32);
	for(i = 0;i < 6;i++) ne[num].mac[i] = eeprom[i * 2];
	ne_outb(num, NE_P0_ISR, 0);
	return 0;
}

static int32_t ne_transmit(int32_t num, int32_t size) {
	int32_t	packetsize;

	while(ne_inb(num, NE_P0_COMMAND) & NE_CR_TXP);
	packetsize = size;
	if(packetsize < ETHER_MIN_PACKET) packetsize = ETHER_MIN_PACKET;
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS0 | NE_CR_RD2 | NE_CR_STA);
	ne_outw(num, NE_P0_TBCR0, packetsize);
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS0 | NE_CR_TXP | NE_CR_RD2 | NE_CR_STA);
	while(ne_inb(num, NE_P0_COMMAND) & NE_CR_TXP);
	return (ne_inb(num, NE_P0_TSR) & NE_TSR_PTX) ? 0 : -1;
}

uint32_t ne_get_config(int32_t num) {
	uint32_t	i, data;

	ne_outb(num, NE_P0_COMMAND, NE_CR_PS3);
	data = 0;
	for(i = 0;i < 4;i++) {
		data <<= 8;
		data |= (uint32_t)ne_inb(num, NE_P3_CONFIG0 + i) & 0xff;
	}
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS0);
	return data;
}

static void ne_ring_init(int32_t num) {
	int32_t	page;

	page = ne[num].start >> 8;
	ne_outb(num, NE_P0_TPSR, page);
	page = ne[num].rx_start >> 8;
	ne_outb(num, NE_P0_BNRY, page);
	ne_outb(num, NE_P0_PSTART, page);
	page = ne[num].rx_end >> 8;
	ne_outb(num, NE_P0_PSTOP, page);
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS1 | NE_CR_STP);
	page = ne[num].rx_start >> 8;
	page++;
	ne_outb(num, NE_P1_CURR, page);
	ne_outb(num, NE_P0_COMMAND, NE_CR_STP);
}

static void ne_setup(int32_t num) {
	int32_t	m;

	ne[num].rx_start = ne[num].start + NE_TX_PAGE_SIZE;
	ne[num].rx_end = ne[num].start + ne[num].size;
	ne_outb(num, NE_P0_COMMAND, NE_CR_STP);
	ne_ring_init(num);
	ne_outb(num, NE_P0_ISR, 0xff);
	ne_outb(num, NE_P0_IMR, NE_IMR_PRXE | NE_IMR_RXEE | NE_IMR_OVWE | NE_IMR_CNTE);
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS1 | NE_CR_STP);
	for(m = 0;m < 6;m++) ne_outb(num, NE_P1_PAR0 + m, ne[num].mac[m]);
	for(m = 0;m < 8;m++) ne_outb(num, NE_P1_MAR0 + m, 0);
	ne_outb(num, NE_P0_COMMAND, NE_CR_STP);
}

static int32_t ne_attach(int32_t num) {
	int32_t	ret;

	ret = ne_probe(num);
	if(ret == 0) ne_setup(num);
	return ret;
}

void int_ne(int32_t num) {
	int32_t fd;
	NE_ISR	status;

	fd = (int32_t)ne[num].fd & 0xffff;
	status.BYTE = ne_inb(num, NE_P0_ISR);
	if(status.BIT.CNT || status.BIT.OVW || status.BIT.RXE) {
		ne_stop(num);
		ne_reset(num);
		ne_setup(num);
		ne_start(num);
	} else if(ne[num].handle != 0 && status.BIT.PRX) {
		(*(ne[num].handle))(fd);
	}
	ne_outb(num, NE_P0_ISR, 0xff);
}

void init_ne() {
	int32_t	num;

	for(num = 0;num < NE_NUM;num++) {
		ne[num].id = 0;
		ne[num].handle = 0;
	}
}

int32_t open_ne(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	SysInfo	 config;
	uint32_t i, ret;
	Byte	 name[4];

	if(num >= NE_NUM) return -1;
	if(ne[num].id != 0) return -1;
	strcpy(name, "ne0");
	name[2] = '0' + num;
	ret = -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, name, 4) == 0) {
			ret = 0;
			ne[num].base = (Byte*)config.address;
			ne[num].irq = config.irq;
			set_bussize(config.cs, config.bus);
			get_config(i + 1, &config);
			if(memcmp(config.name, "pcmcia", 6) == 0) {
				set_bussize(config.cs, config.bus);
				set_wait(config.cs);
				if(pccard_init(config.address) != 0) return -1;
			}
			break;	
		}
	}
	if(ret == -1) return -1;
	if(ne_attach(num) == -1) return -1;
	if(alloc_irq(ne[num].irq, int_ne, num) == -1) return -1;
	ne[num].id = id;
	ne[num].fd = fd;
	ne[num].handle = 0;
	ne_start(num);
	ne_outb(num, NE_P0_ISR, 0xff);
	return 0;
}

int32_t close_ne(uint16_t id, int32_t num) {
	if(num >= NE_NUM) return -1;
	if(ne[num].id != id && ne[num].id != 1) return -1;
	free_irq(ne[num].irq);
	ne_outb(num, NE_P0_IMR, 0);
	ne[num].id = 0;
	ne[num].handle = 0;
	ne_stop(num);
	return 0;
}

int32_t write_ne(int32_t num, STRING buffer, int32_t size) {
	if(num >= NE_NUM) return -1;
	if(ne[num].id == 0) return -1;
	if(size == 0) return 0;
	if(size >= NE_TX_PAGE_SIZE) return -1;
	ne_writemem(num, buffer, ne[num].start, size);
	return ne_transmit(num, size);
}

int32_t read_ne(int32_t num, Byte *buffer, int32_t size) {
	RSR	 rs_stat;
	NE_ISR	 int_stat;
	uint16_t bound, current, next_bound;
	uint16_t rx_start, rx_len, remain_len, sub_len;
	Byte	 header[4], *data;
	uint32_t value;
	int32_t	 datasize;

	datasize = size;
	if(num >= NE_NUM) return 0;
	if(ne[num].id == 0) return 0;
	ne_outb(num, NE_P0_COMMAND, NE_CR_STA);
	rs_stat.BYTE = ne_inb(num, NE_P0_RSR);
	int_stat.BYTE = ne_inb(num, NE_P0_ISR);
	if(int_stat.BIT.OVW == 1 || int_stat.BIT.CNT == 1) {
		ne_stop(num);
		ne_reset(num);
		ne_setup(num);
		ne_start(num);
		return 0;
	}
	if(rs_stat.BIT.PRX == 0) return 0;
	bound = ne_inb(num, NE_P0_BNRY);
	bound++;
	bound <<= 8;
	if(bound >= ne[num].rx_end) bound = ne[num].rx_start;
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS1);
	current = ne_inb(num, NE_P1_CURR);
	current <<= 8;
	if(current >= ne[num].rx_end) current = ne[num].rx_start;
	ne_outb(num, NE_P0_COMMAND, NE_CR_PS0);
	if(current == bound) return 0;
	ne_readmem(num, bound, header, 4);
	rs_stat.BYTE =  header[0];
	next_bound = (uint16_t)header[1] << 8;
	rx_start = bound + 4;
	rx_len = (uint16_t)header[3] & 0xff;
	rx_len <<= 8;
	rx_len |= (uint16_t)header[2] & 0xff;
	rx_len -= 4;
	if(rx_len >= ETHER_HEADER_SIZE && rx_len <= ETHER_MAX_PACKET) {
		remain_len = rx_len;
		sub_len = ne[num].rx_end - rx_start;
		data = buffer;
		if(sub_len < rx_len) {
			if(datasize < sub_len) {
				ne_readmem(num, rx_start, data, datasize);
				datasize = 0;
			} else {
				ne_readmem(num, rx_start, data, sub_len);
				datasize -= sub_len;
			}
			rx_start = ne[num].rx_start;
			data = &(data[sub_len]);
			remain_len = rx_len - sub_len;
		}
		if(datasize < remain_len) {
			ne_readmem(num, rx_start, data, datasize);
			datasize = 0;
		} else {
			ne_readmem(num, rx_start, data, remain_len);
			datasize -= remain_len;
		}
	} else {
		rx_len = 0;
	}
	bound = (next_bound == ne[num].rx_start) ? ne[num].rx_end : next_bound;
	value = ((bound >> 8) & 0xff) - 1;
	ne_outb(num, NE_P0_BNRY, value);
	return size - datasize;
}

int32_t seek_ne(int32_t num, int32_t position) {
	return -1;
}

int32_t ioctl_ne(int32_t num, int32_t data, int32_t op) {
	int32_t		ret;

	if(num >= NE_NUM) return -1;
	if(ne[num].id == 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		break;
	case NE_GETCONFIG:
		ret = ne_get_config(num);
		break;
	case GET_MAC:
		memcpy((STRING)data, ne[num].mac, 6);
		break;
	case SET_HANDLE:
		ne[num].handle = (void*)data;
		break;
	case MAXPACKET:
		ret = ETHER_MAX_PACKET;
		break;
	default:
		ret = -1;
	}
	return ret;
}
