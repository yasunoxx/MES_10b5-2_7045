/****************************************/
/* MES/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include	"fs.h"
unsigned char	sector_buffer[SECSIZE];
diskinfo	hd0, hd1, ee0;
extern	int	eepsize;
int	eep_write(int, char*, int);
int	eep_read(char*, int, int);
int	data_in_command(unsigned short, char*, int, int);
int	data_out_command(unsigned short, char*, int, int);
void	set_word(unsigned short, unsigned int);
void	set_long(unsigned int, unsigned int);

diskinfo* get_device(char *data) {
	if(memcmp(data, "/hd0/", 5) == 0) {
		if(hd0.device_id != HD0) return (diskinfo*)0;
		return &hd0;
	} else if(memcmp(data, "/hd1/", 5) == 0) {
		if(hd1.device_id != HD1) return (diskinfo*)0;
		return &hd1;
	} else if(memcmp(data, "/ee0/", 5) == 0) {
		if(ee0.device_id != EE0) return (diskinfo*)0;
		return &ee0;
	}
	return (diskinfo*)0;
}

int sector_write(unsigned int sector, diskinfo *info) {
	int	i, page;

	switch(info->device_id) {
	case HD0:
		ata_sel_drive(0);
		data_out_command(WriteSector, sector_buffer, sector + info->sector_offset, 1);
		break;
	case HD1:
		ata_sel_drive(1);
		data_out_command(WriteSector, sector_buffer, sector + info->sector_offset, 1);
		break;
	case EE0:
		if(eepsize == 0) return -1;
		if(eepsize < 0x10000) page = 64;
		else page = 256;
		for(i = 0;i < SECSIZE;i += page)
			eep_write(sector * SECSIZE + i, &(sector_buffer[i]), page);
		break;
	default:
		return -1;
	}
	return 0;
}

int sector_read(unsigned int sector, diskinfo *info) {
	int	i, page;

	switch(info->device_id) {
	case HD0:
		ata_sel_drive(0);
		data_in_command(ReadSector, sector_buffer, sector + info->sector_offset, 1);
		break;
	case HD1:
		ata_sel_drive(1);
		data_in_command(ReadSector, sector_buffer, sector + info->sector_offset, 1);
		break;
	case EE0:
		if(eepsize == 0) return -1;
		if(eepsize < 0x10000) page = 64;
		else page = 256;
		for(i = 0;i < SECSIZE;i += page)
			eep_read(&(sector_buffer[i]), sector * SECSIZE + i, page);
		break;
	default:
		return -1;
	}
	return 0;
}

int mbr_read(int id, int offset) {
	diskinfo info;

	info.device_id = id;
	info.sector_offset = offset;
	return sector_read(0, &info);
}

int format(char *data) {
	int	i, fat_sec;

	if(memcmp(data, "/ee0", 4) == 0) {
		if(eepsize == 0) return -1;
		ee0.device_id = EE0;
		ee0.fat = FAT12;
		ee0.sector_size = SECSIZE;
		ee0.reserve_sector = 1;
		ee0.sector_per_clustor = 1;
		ee0.fat_number = 2;
		ee0.root_dir_entry = (ee0.sector_size * 8) / ENTSIZ;
		ee0.media_sector = eepsize / ee0.sector_size;
		fat_sec = ee0.media_sector / ee0.sector_per_clustor;
		fat_sec = fat_sec / ee0.sector_size;
		fat_sec = (fat_sec * 3) / 2;
		ee0.fat_sector_size = fat_sec;
		ee0.media_ident = 0xf8;

		sector_buffer[0] = 0xeb;
		sector_buffer[0x1fe] = 0x55;
		sector_buffer[0x1ff] = 0xaa;
		memcpy(&(sector_buffer[SIG]), "FAT12   ", 8);
		set_word(ee0.sector_size, SS);
		sector_buffer[SC] = ee0.sector_per_clustor;
		set_word(ee0.reserve_sector, RSC);
		sector_buffer[FN] = ee0.fat_number;
		sector_buffer[MEDIA_IDENT] = ee0.media_ident;
		set_word(ee0.root_dir_entry, RDE);
		set_word(ee0.fat_sector_size, SF);
		set_word(ee0.media_sector, TS);
		sector_write(0, &ee0);
		bzero(sector_buffer, SECSIZE);
		for(i = 2;i < ee0.media_sector;i++) sector_write(i, &ee0);
		sector_buffer[0] = 0xff;
		sector_buffer[1] = 0xff;
		sector_buffer[2] = 0xff;
		sector_write(ee0.reserve_sector, &ee0);
		sector_write(ee0.reserve_sector + ee0.fat_sector_size, &ee0);
	}
	return 0;
}
