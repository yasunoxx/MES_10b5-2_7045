void	init_i2c(void);
int32_t open_i2c(uint16_t, int32_t, int32_t, int32_t);
int32_t close_i2c(uint16_t, int32_t);
int32_t write_i2c(int32_t, STRING, int32_t);
int32_t read_i2c(int32_t, STRING, int32_t);
int32_t seek_i2c(int32_t, int32_t);
int32_t ioctl_i2c(int32_t, int32_t, int32_t);

typedef struct {
	int16_t		id, timeover;
	volatile Byte	*dr, *ddr;
	volatile Byte	sda, scl;
	int8_t		addrsize;
} I2CInfo;

#define NAK	SDA
#define ACK	0
#define I2CDDR	(*(i2c[num].ddr))
#define I2CDR	(*(i2c[num].dr))
#define SDA	(i2c[num].sda)
#define SCL	(i2c[num].scl)
