/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"
#include "const.h"
#include "regdef.h"

#define STDSIO	1
#define WBUFSIZ	96     // AT
#define RBUFSIZ 513    // AT

typedef volatile struct {
	union {
		Byte BYTE;
		struct {
			Bits CA  :1;
			Bits CHR :1;
			Bits PE  :1;
			Bits OE  :1;
			Bits STOP:1;
			Bits MP  :1;
			Bits CKS :2;
		} BIT;
	} SMR;
	Byte BRR;
	union {
		Byte BYTE;
		struct {
			Bits TIE :1;
			Bits RIE :1;
			Bits TE  :1;
			Bits RE  :1;
			Bits MPIE:1;
			Bits TEIE:1;
			Bits CKE :2;
		} BIT;
	} SCR;
	Byte TDR;
	union {
		Byte BYTE;
		struct {
			Bits TDRE:1;
			Bits RDRF:1;
			Bits ORER:1;
			Bits FER :1;
			Bits PER :1;
			Bits TEND:1;
			Bits MPB :1;
			Bits MPBT:1;
		} BIT;
	} SSR;
	Byte RDR;
	union {
		Byte BYTE;
		struct {
			Bits     :4;
			Bits SDIR:1;
			Bits SINV:1;
			Bits     :1;
			Bits SMIF:1;
		} BIT;
	} SCMR;
} sci_reg;

typedef struct {
	Byte		current;
	uint16_t	id, freq;
	RingInfo	winfo;
	RingInfo	rinfo;
	Byte		writebuf[WBUFSIZ];  // AT
	Byte		readbuf[RBUFSIZ];   // AT
} SCIInfo;
static SCIInfo sci[SCINUM];
static int32_t ioctl_sci_speed(int32_t, int32_t);

void int_txi(int32_t num) {
	uint32_t	 n, addr;
	Byte		 c;
	sci_reg 	*SCI;

	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg*)addr;
	if(sci[num].current == '\n') {
		sci[num].current = '\r';
		SCI->TDR = '\r';
		SCI->SSR.BIT.TDRE = 0;
	} else {
		n = read_ring(&(sci[num].winfo), &c, 1);
		if(n == 1) {
			sci[num].current = c;
			SCI->TDR = c;
			SCI->SSR.BIT.TDRE = 0;
		} else {
			SCI->SCR.BIT.TIE = 0;
		}
	}
}

void int_eri(int32_t num) {
	uint32_t	 addr;
	sci_reg *SCI;

	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg *)addr;
	SCI->SSR.BIT.ORER = 0;
	SCI->SSR.BIT.FER = 0;
	SCI->SSR.BIT.PER = 0;
}

void int_rxi(int32_t num) {
	uint32_t	 addr;
	Byte		 c;
	sci_reg *SCI;

	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg *)addr;
	c = SCI->RDR;
	write_ring(&(sci[num].rinfo), &c, 1);
	SCI->SSR.BIT.RDRF = 0;
}

#if defined(SH_7045)

 #pragma interrupt
 void int_eri0() {
	int_eri(0);
 }
 #pragma interrupt
 void int_rxi0() {
	int_rxi(0);
 }
 #pragma interrupt
 void int_txi0() {
	int_txi(0);
 }
 #pragma interrupt
 void int_eri1() {
	int_eri(1);
 }
 #pragma interrupt
 void int_rxi1() {
	int_rxi(1);
 }
 #pragma interrupt
 void int_txi1() {
	int_txi(1);
 }
 #pragma interrupt
 void int_eri2() {
	int_eri(2);
 }
 #pragma interrupt
 void int_rxi2() {
	int_rxi(2);
 }
 #pragma interrupt
 void int_txi2() {
	int_txi(2);
 }

#endif

#if defined(SH_7045)

 #pragma interrupt
 void int_eri3() {
	int_eri(3);
 }
 #pragma interrupt
 void int_rxi3() {
	int_rxi(3);
 }
 #pragma interrupt
 void int_txi3() {
	int_txi(3);
 }

#endif

void init_sci() {
	uint32_t	 i, addr;
	volatile int16_t w;
	sci_reg		 *SCI;
	SysInfo		 config;

	addr = (int32_t)&SCIBASE;
	for(i = 0;i < SCINUM;i++,addr += SCIINC) {
		SCI = (sci_reg *)addr;
		SCI->SMR.BYTE = 0;
		SCI->SCR.BYTE = 0;
		for(w = 0;w < 280;w++);
		SCI->SCR.BIT.RE = 1;
		SCI->SCR.BIT.TE = 1;
		w = SCI->SSR.BYTE;
		SCI->SSR.BIT.TDRE = 1;
		ring_init(&(sci[i].winfo), WBUFSIZ, sci[i].writebuf);  // AT
		ring_init(&(sci[i].rinfo), RBUFSIZ, sci[i].readbuf);   // AT
		sci[i].id = 0;
		sci[i].freq = 0;
		sci[i].current = 0;
	}
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, "sci", 3) == 0 && config.bus < SCINUM) {
			sci[config.bus].freq = config.irq;
			ioctl_sci_speed(config.bus, config.address);
		}
	}
}

int32_t write_sci(int32_t num, STRING buffer, int32_t siz) {
	Byte		 c;
	uint32_t	 i, addr, size, max;
	sci_reg *SCI;

	if(num >= SCINUM) return -1;
	if(sci[num].id == 0) return -1;
	if(siz == 0) return 0;
	max = WBUFSIZ - sci[num].winfo.datasize;  // AT
	size = siz;
	if(siz > max) size = max;
	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg *)addr;
	write_ring(&(sci[num].winfo), buffer, size);
	if(SCI->SSR.BIT.TDRE == 1) {
		read_ring(&(sci[num].winfo), &c, 1);
		sci[num].current = c;
		SCI->TDR = c;
		SCI->SSR.BIT.TDRE = 0;
	}
	SCI->SCR.BIT.TIE = 1;
	return size;
}

int32_t read_sci(int32_t num, Byte *buffer, int32_t size) {
	int32_t	n;

	if(num >= SCINUM) return -1;
	if(sci[num].id == 0) return -1;
	if(size == 0) return 0;
	n = read_ring(&(sci[num].rinfo), buffer, size);
	return n;
}

int32_t open_sci(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	uint32_t	 addr;
	sci_reg *SCI;

	if(num >= SCINUM) return -1;
	if(sci[num].id != 0) return -1;
	sci[num].id = id;
	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg*)addr;
	SCI->SCR.BIT.RIE = 1;
	return 0;
}

int32_t close_sci(uint16_t id, int32_t num) {
	uint32_t	 addr;
	sci_reg *SCI;

	if(num >= SCINUM) return -1;
	if(sci[num].id != id && sci[num].id != 1) return -1;
	sci[num].id = 0;
	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg*)addr;
	SCI->SCR.BIT.RIE = 0;
	return 0;
}

int32_t seek_sci(int32_t num, int32_t position) {
	if(num >= SCINUM) return -1;
	if(sci[num].id == 0) return -1;
	return 0;
}

static int32_t ioctl_sci_freq(int32_t num, int32_t freq) {
	sci[num].freq = freq;
	return 0;
}

static int32_t ioctl_sci_speed(int32_t num, int32_t speed) {
	uint32_t	 i, x, y, b, s, addr;
	sci_reg *SCI;

	addr = (int32_t)&SCIBASE + num * SCIINC;
	SCI = (sci_reg*)addr;
	s = 32;
	for(i = 0;i < 4;i++) {
		x = sci[num].freq * 10000000;
		y = s * speed;
		b = x / y + 5;
		b = b / 10 - 1;
		if(b <= 255) break;
		s <<= 2;
	}
	if(i == 4) return -1;
	SCI->SMR.BIT.CKS = i;
	SCI->BRR = b;
	return 0;
}

int32_t ioctl_sci(int32_t num, int32_t data, int32_t op) {
	int32_t	i, ret;

	if(num >= SCINUM) return -1;
	if(sci[num].id == 0) return -1;
	switch(op) {
	case DEV_INFO:
		ret = 0;
		break;
	case SCI_FREQ:
		ret = ioctl_sci_freq(num, data);
		break;
	case SCI_SPEED:
		ret = ioctl_sci_speed(num, data);
		break;
	case SCI_GETID:
		ret = sci[num].id;
	default:
		ret = -1;
	}
	return ret;
}
