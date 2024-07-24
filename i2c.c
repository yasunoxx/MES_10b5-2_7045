/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "const.h"
#include "pubtype.h"
#include "i2c.h"

#define I2C_NUM	4
static I2CInfo i2c[I2C_NUM];

static void start(int32_t num) {
	I2CDDR = SDA | SCL;
	I2CDR &= ~SCL;
	I2CDR |= SDA;
	I2CDR |= SCL;
	I2CDR &= ~SDA;
	I2CDR &= ~SCL;
	I2CDDR = SCL;
}

static void stop(int32_t num) {
	I2CDDR = SDA | SCL;
	I2CDR &= ~SCL;
	I2CDR &= ~SDA;
	I2CDR |= SCL;
	I2CDR |= SDA;
	I2CDR &= ~SCL;
	I2CDDR = SCL;
}

static int writebyte(int32_t num, int32_t data) {
	int	i, mask;

	I2CDDR = SDA | SCL;
	mask = 0x80;
	for(i = 0;i < 8;i++) {
		if(data & mask) I2CDR |= SDA;
		else I2CDR &= ~SDA;
		I2CDR |= SCL;
		I2CDR &= ~SCL;
		mask >>= 1;
	}
	I2CDDR = SCL;
	I2CDR |= SCL;
	i = I2CDR & SDA;
	I2CDR &= ~SCL;
	return i;
}

static int readbyte(int32_t num) {
	int32_t	i, data;

	data = 0;
	for(i = 0;i < 8;i++) {
		data <<= 1;
		I2CDR |= SCL;
		if(I2CDR & SDA) data++;
		I2CDR &= ~SCL;
	}
	return data;
}

static void ack(int32_t num) {
	I2CDDR = SDA | SCL;
	I2CDR &= ~SDA;
	I2CDR |= SCL;
	I2CDR &= ~SCL;
	I2CDDR = SCL;
}

static void nak(int32_t num) {
	I2CDDR = SDA | SCL;
	I2CDR |= SDA;
	I2CDR |= SCL;
	I2CDR &= ~SCL;
	I2CDDR &= ~SDA;
	I2CDDR = SCL;
}

static int status(int32_t num) {
	return I2CDR & SDA;	
}

static int send_ctrl(int32_t num, int32_t ctrl) {
	int32_t	ret, count;

	count = 0;
	do {
		start(num);
		ret = writebyte(num, ctrl);
		if(count++ > i2c[num].timeover) return -1;
	} while(ret == NAK);
	return 0;
}

void init_i2c() {
	int32_t	i;

	for(i = 0;i < I2C_NUM;i++) i2c[i].id = 0;
}

int32_t open_i2c(uint16_t id, int32_t num, int32_t fd, int32_t option) {
	SysInfo2 config;
	int32_t	 i, ret;
	Byte	 name[6];

	if(num >= I2C_NUM || id == 0) return -1;
	if(i2c[num].id != 0) return -1;
	strcpy(name, "i2c0");
	name[3] = '0' + num;
	ret = -1;
	for(i = 0;get_config(i, &config);i++) {
		if(memcmp(config.name, name, 4) == 0) {
			ret = 0;
			i2c[num].sda = config.bus;
			i2c[num].scl = config.cs;
			i2c[num].dr = (volatile char *)config.address;
			i2c[num].ddr = (volatile char *)config.size;
			break;	
		}
	}
	if(ret == -1) return -1;
	i2c[num].id = id;
	i2c[num].timeover = 0;
	i2c[num].addrsize = 0;
	I2CDDR = SCL;
	I2CDR &= ~SCL;
	return 0;
}

int32_t close_i2c(uint16_t id, int32_t num) {
	if(num >= I2C_NUM || id != i2c[num].id) return -1;
	i2c[num].id = 0;
	I2CDDR = SCL | SDA;
	I2CDR = 0;
	I2CDDR = 0;
	return 0;	
}

int32_t write_i2c(int32_t num, STRING src, int32_t size) {
	int32_t	i;

	if(num >= I2C_NUM || i2c[num].id == 0) return -1;
	if(size == 0) return 0;
	for(i = 0;i < size;i++) writebyte(num, src[i]);
	stop(num);
	return size;
}

int32_t read_i2c(int32_t num, STRING dest, int32_t size) {
	int32_t	i;

	if(num >= I2C_NUM || i2c[num].id == 0) return -1;
	if(size == 0) return 0;
	for(i = 0;i < size - 1;i++) {
		dest[i] = readbyte(num);
		ack(num);
	}
	dest[size - 1] = readbyte(num);
	nak(num);
	stop(num);
	return size;
}

int32_t seek_i2c(int32_t num, int32_t addr) {
	int32_t	i, as;

	if(num >= I2C_NUM || i2c[num].id == 0) return -1;
	as = i2c[num].addrsize;
	for(i = as - 1;i >= 0;i--) writebyte(num, addr >> (8 * i));
	return addr;
}

int32_t ioctl_i2c(int32_t num, int32_t data, int32_t op) {
	int32_t ret;

	if(num >= I2C_NUM || i2c[num].id == 0) return -1;
	ret = 0;
	switch(op) {
	case DEV_INFO:
		break;
	case I2C_COMMAND:
		ret = send_ctrl(num, data);
		break;
	case I2C_ADDRSIZE:
		i2c[num].addrsize = data;
		break;
	case I2C_TIMEOVER:
		i2c[num].timeover = data;
		break;
	default:
		ret = -1;
	}
	return ret;
}
