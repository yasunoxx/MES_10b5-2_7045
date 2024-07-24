typedef struct {
	void	(*init_func)();
	int32_t	(*open_func)(uint16_t, int32_t, int32_t, int32_t);
	int32_t	(*close_func)(uint16_t, int32_t);
	int32_t	(*write_func)(int32_t, STRING, int32_t);
	int32_t	(*read_func)(int32_t, STRING, int32_t);
	int32_t	(*seek_func)(int32_t, int32_t);
	int32_t	(*ioctl_func)(int32_t, int32_t, int32_t);
	Byte	name[7];
	Byte	major;
} device_info;

#include "sci.h"
#include "lcd.h"
#include "romdisk.h"
#include "ne.h"
// #include "wi.h"
#include "i2c.h"
#include "sl.h"
#include "seeprom.h"
#include "ramdisk.h"
#define SCI_MAJOR	1
#define LCD_MAJOR	2
#define LCDS_MAJOR	3
#define ROM_MAJOR	4
#define NE_MAJOR	5
#define RAM_MAJOR	6
#define I2C_MAJOR	7
#define SEEP_MAJOR	8
#define SL811_MAJOR	9
// #define WI_MAJOR	10
//#define	ADHOC_MAJOR	11
#define DEVNUM		9

const static device_info DevDef[] = {
  {init_sci,open_sci,close_sci,write_sci,read_sci,seek_sci,ioctl_sci,"sci",SCI_MAJOR},
  {init_lcd,open_lcd,close_lcd,write_lcd,read_lcd,seek_lcd,ioctl_lcd,"lcd",LCD_MAJOR},
  {init_lcd,open_lcdS,close_lcd,write_lcd,read_lcd,seek_lcd,ioctl_lcd,"lcdS",LCDS_MAJOR},
  {init_rom,open_rom,close_rom,write_rom,read_rom,seek_rom,ioctl_rom,"rom",ROM_MAJOR},
  {init_ne,open_ne,close_ne,write_ne,read_ne,seek_ne,ioctl_ne,"ne",NE_MAJOR},
  {init_ram,open_ram,close_ram,write_ram,read_ram,seek_ram,ioctl_ram,"ram",RAM_MAJOR},
  {init_i2c,open_i2c,close_i2c,write_i2c,read_i2c,seek_i2c,ioctl_i2c,"i2c",I2C_MAJOR},
  {init_se,open_se,close_se,write_se,read_se,seek_se,ioctl_se,"se",SEEP_MAJOR},
  {init_sl811,open_sl811,close_sl811,write_sl811,read_sl811,seek_sl811,ioctl_sl811,"sl",SL811_MAJOR},
//  {init_wi,open_wi,close_wi,write_wi,read_wi,seek_wi,ioctl_wi,"wi",WI_MAJOR},
//  {init_wi,open_wa,close_wi,write_wi,read_wi,seek_wi,ioctl_wi,"wa",ADHOC_MAJOR},
};
