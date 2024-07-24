/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/12/12 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"
#include "const.h"
#include "sl811.h"

#define SL_NUM	1
typedef struct {
	uint16_t	id;
	volatile Byte	*addr_reg;
	volatile Byte	*data_reg;
	Byte		speed;
	Byte		addr;
	Byte		irq;
} SL811Info;
static SL811Info sl811[SL_NUM];
#define USB_ACK		1
#define USB_NAK		2
#define USB_STALL	4

static void waitms(int32_t ms) {
	volatile int32_t	a, b;

	for(a = 0;a < ms;a++) {
		for(b = 0;b < 2000;b++);
	}
}

static void waitus(int32_t us) {
	volatile int32_t	a;

	for(a = 0;a < us*2;a++);
}

static Byte SL811Read(int32_t num, Byte offset) {
	Byte data;

	*(sl811[num].addr_reg) = offset;
	data = *(sl811[num].data_reg);
	return data;
}

static void SL811Write(int32_t num, Byte offset, Byte data){
	*(sl811[num].addr_reg) = offset;
	*(sl811[num].data_reg) = data;
}

static void SL811BufRead(int32_t num, int16_t offset, STRING buf, int16_t size){
	if(size <= 0) return;
	*(sl811[num].addr_reg) = (Byte)offset;
	while(size--) *buf++ = *(sl811[num].data_reg);
}

static void SL811BufWrite(int32_t num, int16_t offset, STRING buf, int16_t size) {
	if(size <= 0) return;
	*(sl811[num].addr_reg) = (Byte)offset;
	while(size--) {
		*(sl811[num].data_reg) = *buf;
		buf++;
	}
}

static int32_t regTest(int32_t num){
	int32_t	i, data, result = 0;
	Byte	buf[256];

	for(i = 16;i < 256; i++) {
		buf[i] = (Byte)SL811Read(num, i);
		SL811Write(num, i, i);
	}
	for(i = 16;i < 256; i++) {
		data = SL811Read(num, i);
		if(data != i) {
			result = -1;
		}
	}
	for(i = 16;i < 256;i++) SL811Write(num, i, buf[i]);
	return result;
}

static int32_t USBReset(int32_t num) {
	int32_t	status;

	SL811Write(num, SL11H_CTLREG2, 0xae);
	SL811Write(num, SL11H_CTLREG1, 0x08);	// reset USB
	waitms(20);				// 20ms
	SL811Write(num, SL11H_CTLREG1, 0);	// remove SE0

	for(status = 0;status < 100;status++) {
		SL811Write(num, SL11H_INTSTATREG, 0xff); // clear all interrupt bits
	}
	status = SL811Read(num, SL11H_INTSTATREG);
	if(status & 0x40){  // Check if device is removed
		sl811[num].speed = 0;	// None
		SL811Write(num, SL11H_INTENBLREG, SL11H_INTMASK_XFERDONE | 
			   SL11H_INTMASK_SOFINTR | SL11H_INTMASK_INSRMV);
		return -1;
	}
	SL811Write(num, SL11H_BUFLNTHREG_B, 0);	//zero lenth
	SL811Write(num, SL11H_PIDEPREG_B, 0x50);	//send SOF to EP0
	SL811Write(num, SL11H_DEVADDRREG_B, 0x01);	//address0
	SL811Write(num, SL11H_SOFLOWREG, 0xe0);
	if(!(status & 0x80)) {
		sl811[num].speed = USB_LOW;	// Low
		SL811Write(num, SL11H_CTLREG1, 0x8);
		waitms(20);
		SL811Write(num, SL11H_SOFTMRREG, 0xee);
		SL811Write(num, SL11H_CTLREG1, 0x21);
		SL811Write(num, SL11H_HOSTCTLREG_B, 0x01);
		for(status = 0;status < 20;status++) {
			SL811Write(num, SL11H_INTSTATREG, 0xff);
		}
	} else {
		sl811[num].speed = USB_FULL;	// Full
		SL811Write(num, SL11H_CTLREG1, 0x8);
		waitms(20);
		SL811Write(num, SL11H_SOFTMRREG, 0xae);
		SL811Write(num, SL11H_CTLREG1, 0x01 );
		SL811Write(num, SL11H_HOSTCTLREG_B, 0x01);
		SL811Write(num, SL11H_INTSTATREG, 0xff);
	}
	SL811Write(num, SL11H_INTENBLREG, SL11H_INTMASK_XFERDONE | 
		   SL11H_INTMASK_SOFINTR|SL11H_INTMASK_INSRMV);
	return 0;
}

static int32_t write_setup(int32_t num, STRING data) {
	Byte	pktstat, ret;

	SL811BufWrite(num, 0x10, data, 8);
	SL811Write(num, SL11H_BUFADDRREG, 0x10);
	SL811Write(num, SL11H_BUFLNTHREG, 8);
	SL811Write(num, SL11H_DEVADDRREG, sl811[num].addr);
	SL811Write(num, SL11H_PIDEPREG, PID_SETUP);
	SL811Write(num, SL11H_HOSTCTLREG, DATA0_WR);
	waitus(200);
	while(SL811Read(num, SL11H_INTSTATREG) & SL11H_INTMASK_XFERDONE == 0);
	pktstat = SL811Read(num, SL11H_PKTSTATREG);
	ret = 0;
	if(pktstat & SL11H_STATMASK_ACK) ret |= USB_ACK;
	if(pktstat & SL11H_STATMASK_NAK) ret |= USB_NAK;
	if(pktstat & SL11H_STATMASK_STALL) ret |= USB_STALL;
	return ret;
}

void init_sl811() {
	int32_t	i;

	for(i = 0;i < SL_NUM;i++) sl811[i].id = 0;
}

int32_t open_sl811(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	SysInfo	 config;
	uint32_t i, ret;
	Byte	 name[4];

	if(num >= SL_NUM) return -1;
	if(sl811[num].id != 0) return -1;
	strcpy(name, "sl0");
	name[2] = '0' + num;
	ret = -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, name, 4) == 0) {
			ret = 0;
			sl811[num].addr_reg = (volatile Byte *)(config.address + 1);
			sl811[num].data_reg = (volatile Byte *)(config.address + 3);
			sl811[num].irq = config.irq;
			set_bussize(config.cs, config.bus);
			break;	
		}
	}
	if(ret == -1) return -1;
	if(regTest(num) != 0) return -1;
	sl811[num].id = id;
	sl811[num].addr = 0;
	return 0;
}

int32_t close_sl811(uint16_t id, int32_t num) {
	if(num >= SL_NUM) return -1;
	if(sl811[num].id != id && sl811[num].id != 1) return -1;
	sl811[num].id = 0;
	SL811Write(num, SL11H_CTLREG1, 0);	// remove SE0
	return 0;
}

int32_t write_sl811(int32_t num, STRING data, int32_t size) {
	Byte	pktstat, ret;

	if(size < USB_HDRSIZ) return 0;
	SL811BufWrite(num, 0x10, &data[USB_HDRSIZ], size - USB_HDRSIZ);
	SL811Write(num, SL11H_BUFADDRREG, 0x10);
	SL811Write(num, SL11H_BUFLNTHREG, size - USB_HDRSIZ);
	SL811Write(num, SL11H_DEVADDRREG, sl811[num].addr);
	SL811Write(num, SL11H_PIDEPREG, PID_OUT | data[USB_EP]);
	SL811Write(num, SL11H_HOSTCTLREG, (data[USB_TOGGLE] == 0) ? DATA0_WR : DATA1_WR);
	waitus(200);
	while(SL811Read(num, SL11H_INTSTATREG) & SL11H_INTMASK_XFERDONE == 0);
	pktstat = SL811Read(num, SL11H_PKTSTATREG);
	ret = 0;
	if(pktstat & SL11H_STATMASK_ACK) ret |= USB_ACK;
	if(pktstat & SL11H_STATMASK_NAK) ret |= USB_NAK;
	if(pktstat & SL11H_STATMASK_STALL) ret |= USB_STALL;
	return ret;
}

int32_t read_sl811(int32_t num, STRING data, int32_t size) {
	Byte	pktstat, ret;
	int32_t	timovr;

	if(size < 0) return 0;
	SL811Write(num, SL11H_BUFADDRREG, 0x10);
	SL811Write(num, SL11H_BUFLNTHREG, size);
	SL811Write(num, SL11H_DEVADDRREG, sl811[num].addr);
	SL811Write(num, SL11H_PIDEPREG, PID_IN | data[USB_EP]);
	for(timovr = 0;timovr < 200;timovr++) {
		SL811Write(num, SL11H_HOSTCTLREG, (data[USB_TOGGLE] == 0) ? DATA0_RD : DATA1_RD);
		waitus(200);
		while(SL811Read(num, SL11H_INTSTATREG) & SL11H_INTMASK_XFERDONE == 0);
		pktstat = SL811Read(num, SL11H_PKTSTATREG);
		if(!(pktstat & SL11H_STATMASK_NAK)) break;
	}
	if(pktstat & SL11H_STATMASK_ACK) SL811BufRead(num, 0x10, data, size);
	ret = 0;
	if(pktstat & SL11H_STATMASK_ACK) ret |= USB_ACK;
	if(pktstat & SL11H_STATMASK_NAK) ret |= USB_NAK;
	if(pktstat & SL11H_STATMASK_STALL) ret |= USB_STALL;
	return ret;
}

int32_t seek_sl811(int32_t num, int32_t position) {
	if(num >= SL_NUM) return -1;
	if(sl811[num].id == 0) return -1;
	sl811[num].addr = position;
	return 0;
}

int32_t ioctl_sl811(int32_t num, int32_t data, int32_t op) {
	static Byte	getdesc[8] = {0x80, 0x06, 0, 1, 0, 0, 64, 0};
	static Byte	setaddr[8] = {0x00, 0x05, 2, 0, 0, 0, 0, 0};
	Byte		buf[10];
	int32_t		ret;

	if(num >= SL_NUM) return -1;
	ret = 0;
	switch(op) {
	case USB_RESET:
		ret = -1;
		sl811[num].addr = 0;
		if(USBReset(num) != 0) break;
		if((write_setup(num, getdesc) & USB_ACK) == 0) break;
		buf[USB_EP] = 0, buf[USB_TOGGLE] = 1;
		if((read_sl811(num, buf, 8) & USB_ACK) == 0) break;
		buf[USB_EP] = 0, buf[USB_TOGGLE] = 1;
		if((write_sl811(num, buf, 2) & USB_ACK) == 0) break;
		if((write_setup(num, setaddr) & USB_ACK) == 0) break;
		buf[USB_EP] = 0, buf[USB_TOGGLE] = 1;
		if((read_sl811(num, buf, 0) & USB_ACK) == 0) break;
		sl811[num].addr = data;
		ret = buf[7];
		break;
	case USB_GETSPEED:
		ret = sl811[num].speed;
		break;
	case USB_SETUP:
		ret = write_setup(num, (STRING)data);
		break;
	default:
		ret = -1;
	}
	return ret;
}
