/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/12/07 user font loading	*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <ctype.h>
#include <string.h>
#include "macro.h"
#include "const.h"
#include "pubtype.h"
#include "lcddef.h"

static void serial_out(int32_t, int32_t);
static void outlcd(int32_t, int32_t);
static void Clear(int32_t);
static void Home(int32_t);
static void Mode(int32_t, int32_t);
static void Control(int32_t, int32_t);
static void Cursor(int32_t, int32_t);
static void FuncSet(int32_t);
static void PutChar(int32_t, int32_t);
static void Locate(int32_t, int32_t);
static void LcdInit(int32_t);
static void fix_current(int32_t);
static int32_t lcd_putch(int32_t, Byte);
static void lcd_clear(int32_t);
static void lcdsetup(int32_t, int32_t, int32_t, IOREG *, IOREG *);

#define LCDNUM	2
typedef struct {
	uint16_t id;
	uint16_t current;
	uint16_t max_low;
	uint16_t max_column;
	IOREG	 masks[7];
	IOREG	 masks164[3];
	IOREG	 *lcdreg;
	Byte	 offset;
	Byte	 databuf[2][40];
} LCDInfo;
static LCDInfo lcd[LCDNUM];

static void serial_out(int32_t num, int32_t data) {
	int32_t	i;

	*(lcd[num].lcdreg) &= ~(DATA | CLK);
	for(i = 0;i < 8;i++) {
		if(data & 0x80) *(lcd[num].lcdreg) |= DATA;
		else *(lcd[num].lcdreg) &= ~DATA;
		*(lcd[num].lcdreg) |= CLK;
		*(lcd[num].lcdreg) &= ~CLK;
		data <<= 1;
	}
}

static void outlcd(int32_t num, int32_t data) {
	volatile int32_t	i;

	if(SerialMode) {
		serial_out(num, data);
		*(lcd[num].lcdreg) |= ENA;
		for(i = 0;i < 10*WAITSCALE;i++);
		*(lcd[num].lcdreg) &= ~ENA;
		for(i = 0;i < 80*WAITSCALE;i++);
	} else {
		*(lcd[num].lcdreg) = CS | data;
		for(i = 0;i < 10*WAITSCALE;i++);
		*(lcd[num].lcdreg) = data;
		for(i = 0;i < 80*WAITSCALE;i++);
	}
}

static void Clear(int32_t num) {
	volatile int32_t i;

	outlcd(num, NONE); outlcd(num, D0);
	for(i = 0;i < 3600*WAITSCALE;i++);	
}

static void Home(int32_t num) {
	volatile int32_t i;

	outlcd(num, NONE); outlcd(num, D1);
	for(i = 0;i < 3600*WAITSCALE;i++);	
}

static void Mode(int32_t num, int32_t com) {
	outlcd(num, NONE); outlcd(num, D2 | (com & (D0 | D1)));
}

static void Control(int32_t num, int32_t com) {
	outlcd(num, NONE); outlcd(num, D3 | (com & (D0 | D1 | D2)));
}

static void Cursor(int32_t num, int32_t com) {
	outlcd(num, D4); outlcd(num, com & (D3 | D2));
}

static void FuncSet(int32_t num) {
	outlcd(num, D5); outlcd(num, D3);
}

static void PutChar(int32_t num, int32_t c) {
	volatile int32_t i;
	int32_t		 hi, low;

	hi = low = 0;
	if(c & 0x80) hi |= D7;
	if(c & 0x40) hi |= D6;
	if(c & 0x20) hi |= D5;
	if(c & 0x10) hi |= D4;
	if(c & 0x08) low |= D3;
	if(c & 0x04) low |= D2;
	if(c & 0x02) low |= D1;
	if(c & 0x01) low |= D0;
	outlcd(num, RS | hi);outlcd(num, RS | low);
	for(i = 0;i < 150*WAITSCALE;i++);	
}

static void Locate(int32_t num, int32_t number) {
	int32_t	hi, low;

	hi = low = 0;
	if(number & 0x40) hi |= D6;
	if(number & 0x20) hi |= D5;
	if(number & 0x10) hi |= D4;
	if(number & 0x08) low |= D3;
	if(number & 0x04) low |= D2;
	if(number & 0x02) low |= D1;
	if(number & 0x01) low |= D0;
	outlcd(num, D7 | hi);outlcd(num, low);
}

static void CGRAM_SetAddr(int32_t num, int32_t number) {
	int32_t	hi, low;

	hi = low = 0;
	if(number & 0x20) hi |= D5;
	if(number & 0x10) hi |= D4;
	if(number & 0x08) low |= D3;
	if(number & 0x04) low |= D2;
	if(number & 0x02) low |= D1;
	if(number & 0x01) low |= D0;
	outlcd(num, D6 | hi);outlcd(num, low);
}

static void WriteRAM(int32_t num, int32_t code, STRING data) {
	int32_t	hi, low, c;

	Mode(num, LCDINC);
	CGRAM_SetAddr(num, code << 3);
	for(c = 0;c < 8;c++) {
		hi = low = 0;
		if(data[c] & 0x80) hi |= D7;
		if(data[c] & 0x40) hi |= D6;
		if(data[c] & 0x20) hi |= D5;
		if(data[c] & 0x10) hi |= D4;
		if(data[c] & 0x08) low |= D3;
		if(data[c] & 0x04) low |= D2;
		if(data[c] & 0x02) low |= D1;
		if(data[c] & 0x01) low |= D0;
		outlcd(num, RS | hi);outlcd(num, RS | low);
	}
	Mode(num, NONE);
	outlcd(num, D7);outlcd(num, 0);
}

static void LcdInit(int32_t num) {
	volatile int32_t	i;

	for(i = 0;i < 100000*WAITSCALE;i++);
	outlcd(num, D5 | D4);
	for(i = 0;i < 20000*WAITSCALE;i++);
	outlcd(num, D5 | D4);
	for(i = 0;i < 400*WAITSCALE;i++);	
	outlcd(num, D5 | D4);
	outlcd(num, D5);
	FuncSet(num);
	Control(num, NONE);
	Clear(num);
	Mode(num, NONE);
	Control(num, LCDDISPLAY);
}

static void fix_current(int32_t num) {
	int32_t	i;

	if(lcd[num].current == lcd[num].max_column * 2 + 64) {
		lcd[num].current = lcd[num].max_column + 64;
		Clear(num);
		memcpy(lcd[num].databuf[0], lcd[num].databuf[1], lcd[num].max_column);
		memcpy(lcd[num].databuf[1], &(lcd[num].databuf[0][lcd[num].max_column]), lcd[num].max_column);
		memcpy(&(lcd[num].databuf[0][lcd[num].max_column]), &(lcd[num].databuf[1][lcd[num].max_column]), lcd[num].max_column);
		for(i = 0;i < lcd[num].max_column;i++) {
			Locate(num, i);
			PutChar(num, lcd[num].databuf[0][i]);
			Locate(num, i + 64);
			PutChar(num, lcd[num].databuf[1][i]);
		}
		for(i = 0;i < lcd[num].max_column;i++) {
			Locate(num, i + lcd[num].max_column);
			PutChar(num, lcd[num].databuf[0][i + lcd[num].max_column]);
			lcd[num].databuf[1][i + lcd[num].max_column] = ' ';
		}
	} else if(lcd[num].current == lcd[num].max_column * 2) {
		lcd[num].current = 64 + lcd[num].max_column;
	} else if(lcd[num].current == lcd[num].max_column + 64) {
		if(lcd[num].max_low == 4) {
			lcd[num].current = lcd[num].max_column;
		} else {
			lcd[num].current = 64;
			Clear(num);
			memcpy(lcd[num].databuf[0], lcd[num].databuf[1], lcd[num].max_column);
			for(i = 0;i < lcd[num].max_column;i++) {
				Locate(num, i);
				PutChar(num, lcd[num].databuf[0][i]);
				lcd[num].databuf[1][i] = ' ';
			}
		}
	} else if(lcd[num].current == lcd[num].max_column) {
		lcd[num].current = 64;
	}
}

static int32_t lcd_putch(int32_t num, Byte code) {
	int32_t	i, n;
	Byte	c;

	c = code;
	if(c == 0x7e) c = 0xf3;
	if(c == 0x8e) return -1;
	if(c == 0x0d) return -1;

/*	if(c == 8) {
		if(lcd[num].current == 0) {
		} else if(lcd[num].current == 64) {
			lcd[num].current = lcd[num].max_column - 1;
		} else if(lcd[num].current == lcd[num].max_column) {
			lcd[num].current = lcd[num].max_column - 1 + 64;
		} else if(lcd[num].current == (lcd[num].max_column + 64)) {
			lcd[num].current = lcd[num].max_column * 2 - 1;
		} else {
			lcd[num].current--;
		}
		Locate(num, lcd[num].current);
	}*/

	if(c == '\n') {
		if(lcd[num].current < lcd[num].max_column) lcd[num].current = lcd[num].max_column;
		else if(lcd[num].current < lcd[num].max_column * 2) lcd[num].current = lcd[num].max_column * 2;
		else if(lcd[num].current < 64) lcd[num].current = 0;
		else if(lcd[num].current < 64 + lcd[num].max_column) lcd[num].current = lcd[num].max_column + 64;
		else if(lcd[num].current < lcd[num].max_column * 2 + 64) lcd[num].current = lcd[num].max_column * 2 + 64;
		else lcd[num].current = 0;
		fix_current(num);
		Locate(num, lcd[num].current);
	} else if(c >= 0x20 || c < 0x10) {
		Locate(num, lcd[num].current);
		PutChar(num, c);
		if(lcd[num].current < 64)
			lcd[num].databuf[0][lcd[num].current] = c;
		else
			lcd[num].databuf[1][lcd[num].current - 64] = c;
		lcd[num].current++;
		fix_current(num);
	}
	return 0;
}

static void lcd_clear(int32_t num) {
	lcd[num].current = 0;
	Clear(num);
}

static void lcdsetup(int32_t num, int32_t low, int32_t column, IOREG *regptr, IOREG *mask) {
	int32_t	i, n;

	if(low == 4) lcd[num].max_low = low;
	else lcd[num].max_low = 2;
	lcd[num].max_column = column;
	SerialMode = 0;
	if(mask[5] == 0) {
		for(i = 0;i < 5;i++) lcd[num].masks[i] = mask[i];
		for(i = 0;i < 3;i++) lcd[num].masks164[i] = mask[i + 6];
	} else {
		for(i = 0;i < 6;i++) lcd[num].masks[i] = mask[i];
	}
	lcd[num].lcdreg = regptr;
	*(lcd[num].lcdreg) = 0;
	LcdInit(num);
	lcd[num].current = 0;
	n = (lcd[num].max_low == 4) ? lcd[num].max_column * 2 : lcd[num].max_column;
	for(i = 0;i < n;i++) {
		lcd[num].databuf[0][i] = ' '; 
		lcd[num].databuf[1][i] = ' '; 
	}
}

void init_lcd() {
	int32_t	num;

	for(num = 0;num < LCDNUM;num++) {
		lcd[num].id = 0;
	}
}

int32_t open_lcd(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	LCDConf	config;
	int32_t i, c, ret, sft;
	IOREG	mask[6];

	if(num >= LCDNUM) return -1;
	if(lcd[num].id != 0) return -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, "lcd", 3) == 0 && config.name[3] == (num + '0')) {
			for(c = 0;c < 6;c++) {
				sft = (config.bits >> ((5 - c) * 4)) & 0xf;
				mask[c] = 1 << sft;
			}
			lcdsetup(num, config.low, config.column, (IOREG *)config.address, mask);
 			break;	
		}
	}
	lcd[num].id = id;
	lcd[num].offset = 0;
	return 0;
}

int32_t open_lcdS(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	LCDConf	config;
	int32_t i, c, ret, sft;
	IOREG	mask[6];

	if(num >= LCDNUM) return -1;
	if(lcd[num].id != 0) return -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, "lcdS", 4) == 0 && config.name[4] == (num + '0')) {
			for(c = 0;c < 8;c++) {
				sft = (config.bits >> ((5 - c) * 4)) & 0xf;
				mask[c] = 1 << sft;
			}
			for(c = 5;c < 8;c++) mask[c + 1] = mask[c];
			mask[5] = 0;
			lcdsetup(num, config.low, config.column, (IOREG *)config.address, mask);
 			break;	
		}
	}
	lcd[num].id = id;
	return 0;
}

int32_t close_lcd(uint16_t id, int32_t num) {
	if(num >= LCDNUM) return -1;
	if(lcd[num].id != id && lcd[num].id != 1) return -1;
	lcd[num].id = 0;
	return 0;
}

int32_t write_lcd(int32_t num, STRING buffer, int32_t size) {
	int32_t	i;

	for(i = 0;i < size;i++) lcd_putch(num, buffer[i]);
	return size;
}

int32_t read_lcd(int32_t num, Byte *buffer, int32_t size) {
	return -1;
}

int32_t seek_lcd(int32_t num, int32_t position) {
	if(num >= LCDNUM) return -1;
	if(lcd[num].id == 0) return -1;
	lcd[num].offset = position;
	return 0;
}

int32_t ioctl_lcd(int32_t num, int32_t data, int32_t op) {
	int32_t		ret;

	if(num >= LCDNUM) return -1;
	if(lcd[num].id == 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		break;
	case LCD_CLEAR:
		lcd_clear(num);
		break;
	case LCD_SETFONT:
		WriteRAM(num, lcd[num].offset, (STRING)data);
		break;
	default:
		ret = -1;
	}
	return ret;
}
