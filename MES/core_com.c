/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include <string.h>
#include "../syscall.h"
#include "print.h"
#define SuperPID 1
/****************************************/
/* Mini FS				*/
/****************************************/
#define	FATSIZEOF	sizeof(uint16_t)
#define	CACHESIZ	512
#define MAXPATHSIZ	128
#define FREE		0xffff
#define TAIL		0x7fff
#define FD_MASK		0xff
#define	DEV_SHIFT	8
#define	DEV_MASK	0x0f

#define READ_FILE	1
#define APPEND_FILE	2
#define DIR_FILE	4

#define ATTR_DIR	1
typedef struct {
	Byte	 name[24];
	int32_t	 size;
	uint16_t fat;
	uint16_t attr;
} Entry;

typedef struct {
	int32_t	 disksize;
	int32_t	 pagesize;
	uint16_t firstfat;
	int32_t	 fd;
} DiskInfo;
static DiskInfo disk[1];

static void fsinit() {
	int32_t	n, fd;

	disk[0].fd = open_device(SuperPID, "rom0", 0);
	disk[0].disksize = ioctl_device(disk[0].fd, 0, DISK_SIZE);
	disk[0].pagesize = ioctl_device(disk[0].fd, 0, DISK_PAGE);
	n = (disk[0].disksize / disk[0].pagesize) * FATSIZEOF;
	n /= disk[0].pagesize;
	disk[0].firstfat = n;
}

static int new_entry(int32_t num, STRING name, int32_t size, uint16_t fat) {
	int32_t	i, e, en;
	Byte	buffer[disk[num].pagesize];
	Entry	*entries;

	entries = (Entry*)buffer;
	en = disk[num].pagesize / sizeof(Entry);
	for(i = 0;i < 8;i++) {
		seek_device(disk[num].fd, (i + disk[num].firstfat) * disk[num].pagesize);
		read_device(disk[num].fd, buffer, disk[num].pagesize);
		for(e = 0;e < en;e++) {
			if(entries[e].name[0] == 0xff) { 
				strncpy(entries[e].name, name, 23);
				entries[e].size = size;
				entries[e].attr = 0;
				entries[e].fat = fat;
				write_device(disk[num].fd, buffer, disk[num].pagesize);
				return i * en + e;
			}
		}
	}
	return -1;
}

static int del_entry(int32_t num, STRING name) {
	int32_t	i, e, en;
	Byte	buffer[disk[num].pagesize];
	Entry	*entries;

	entries = (Entry*)buffer;
	en = disk[num].pagesize / sizeof(Entry);
	for(i = 0;i < 8;i++) {
		seek_device(disk[num].fd, (i + disk[num].firstfat) * disk[num].pagesize);
		read_device(disk[num].fd, buffer, disk[num].pagesize);
		for(e = 0;e < en;e++) {
			if(strcmp(entries[e].name, name) == 0) {
				bzero((char*)&(entries[e]), sizeof(Entry));
				seek_device(disk[num].fd, (i + disk[num].firstfat) * disk[num].pagesize);
				write_device(disk[num].fd, buffer, disk[num].pagesize);
				return i * en + e;
			}
		}
	}
	return -1;
}

static uint16_t new_fat(int32_t num, int32_t fatnum) {
	int32_t	 f, fn, max, rep, remain;
	Byte	 buffer[disk[num].pagesize];
	uint16_t *fats, next;

	next = TAIL;
	remain = fatnum;
	fats = (short*)buffer;
	max = disk[num].disksize / disk[num].pagesize;
	fn = disk[num].pagesize / sizeof(uint16_t);
	seek_device(disk[num].fd, 0);
	read_device(disk[num].fd, buffer, disk[num].pagesize);
	if(fats[0] == FREE) {
		fats[0] = disk[num].firstfat;
		for(f = disk[num].firstfat;f < (disk[num].firstfat + 7);f++) {
			fats[f] = f + 1;
		}
		fats[f] = TAIL;
	}
	rep = 0;
	for(f = disk[num].firstfat + 8;f < max;f++) {
		if((f % fn) == 0) {
			seek_device(disk[num].fd, (f / fn) * disk[num].pagesize);
			read_device(disk[num].fd, buffer, disk[num].pagesize);
		}
		if(fats[f % fn] == FREE && remain > 0) {
			fats[f % fn] = next;
			next = f;
			rep = 1;
			remain--;
		}
		if((f % fn) == (fn - 1) && rep == 1) {
			seek_device(disk[num].fd, (f / fn) * disk[num].pagesize);
			write_device(disk[num].fd, buffer, disk[num].pagesize);
			rep = 0;
		}
	}
	return (remain == 0) ? next : FREE;
}

static int get_entry(int32_t num, int32_t fd, Entry *ent) {
	Byte	buffer[disk[num].pagesize];
	int32_t	en;
	Entry	*entries;

	entries = (Entry*)buffer;
	en = disk[num].pagesize / sizeof(Entry);
	if((fd / en) >= 8) return -1;
	seek_device(disk[num].fd, (fd / en + disk[num].firstfat) * disk[num].pagesize);
	read_device(disk[num].fd, buffer, disk[num].pagesize);
	memcpy((char*)ent, (char*)&entries[fd % en], sizeof(Entry));
	return 0;
}

static int writedata(int32_t num, int32_t fd, int32_t offset, STRING data, int32_t size) {
	int32_t	 en, off, current, remain, s;
	Byte	 buffer[disk[num].pagesize];
	uint16_t node;
	Entry	 *entries, entry;

	entries = (Entry*)buffer;
	en = disk[num].pagesize / sizeof(Entry);
	seek_device(disk[num].fd, (fd / en + disk[num].firstfat) * disk[num].pagesize);
	read_device(disk[num].fd, buffer, disk[num].pagesize);
	memcpy((char*)&entry, (char*)&entries[fd % en], sizeof(Entry));
	if(entry.size < offset + size) return -1;
	node = entry.fat;
	current = offset;
	remain = size;
	node -= offset / disk[num].pagesize;
	off = (offset / disk[num].pagesize) * disk[num].pagesize;
	for(;off < offset + size;off += disk[num].pagesize) {
		seek_device(disk[num].fd, node * disk[num].pagesize);
		read_device(disk[num].fd, buffer, disk[num].pagesize);
		s = disk[num].pagesize - (current % disk[num].pagesize);
		s = (remain > s) ? s : remain;
		memcpy(&buffer[current % disk[num].pagesize], &data[current - offset], s);
		current += s;
		remain -= s;
		seek_device(disk[num].fd, node * disk[num].pagesize);
		write_device(disk[num].fd, buffer, disk[num].pagesize);
		node--;
	}
	return size - remain;
}

/****************************************/
/* A shell of CoreOS			*/
/****************************************/
#define MAX_BUF	170  // AT 2004/08/25
#define MAX_ARG	50   // AT 2004/08/25
#define NONE	0
#define COMMAND	1
#define SYNTAX	2
#define IOERR	3
#define DSKFULL	4
#define CMDFULL	5
#define NOFILE	6
#define MEMFULL	7
#define IGFMT	8
#define ARIGN	9
#define FSCHK	2
static int32_t	stdio, fsready, filefd;
static Byte	prompt[16], echo_flag;
extern void pci_test();

int32_t	__write(uint32_t fd, STRING data, uint32_t size) {
	return write_device(stdio, data, size);
}

int32_t	__read(uint32_t fd, STRING data, uint32_t size) {
	return read_device(stdio, data, size);
}

static uint32_t read_line(STRING buffer, STRING* argv) {
	uint32_t i, argc;
	STRING	 ptr;

	ptr = buffer;
	for(;;) {
		if((uint32_t)ptr >= (uint32_t)(buffer + MAX_BUF)) ptr = &buffer[MAX_BUF - 1];
		*ptr = getchar();
		if(*ptr >= 0x80) continue;
		if(*ptr == '\r') break;
		if(*ptr == 0x7f) *ptr = 8;
		if(*ptr == 8) {
			if((uint32_t)ptr > (uint32_t)buffer) {
				if(echo_flag) putchar(*ptr);
				ptr--;
			}
		} else {
			if(echo_flag) putchar(*ptr);
			ptr++;
		}
	}
	*ptr = 0;
	if(buffer[0] == 0) return 0;
	ptr = buffer;
	i = argc = 0;
	argv[argc++] = &(buffer[i]);
	while(buffer[i] != 0) {
		if(buffer[i] == ' ') {
			buffer[i] = 0;
			argv[argc++] = &(buffer[i + 1]);
		}
		i++;
	}
	return argc;
}

static void err_message(uint32_t id) {
	static const Byte message[10][20] = {
		"Command not found.",
		"Syntax error.",
		"Disk I/O error.",
		"Disk is full.",
		"Buffer is full.",
		"File not found.",
		"Memory is full.",
		"Ignore file format.",
		"Arignment error.",
		"Unknown error."
	};
	uint32_t	i;

	i = (id > 8) ? 8 : id - 1;
	printf("ERROR[%s]\n", message[i]);
}

static uint32_t dump(uint32_t argc, STRING *argv) {
	uint32_t	  i, n, num;
	volatile uint32_t w;
	int8_t		  *pb, c;
	int16_t		  *pw;

	sscanf(argv[argc - 1], "%x", &num);
	n = num + 16 * 8;
	while(num < n) {
		printf("%08x : ", num);
		if(argc == 3 && argv[1][0] == 'w') {
			if(num & 1) return ARIGN;
			pw = (int16_t*)num;
			for(i = 0;i < 8;i++) {
				if(i == 4) printf("  ");
				printf("%04x", (uint32_t)(*pw++) & 0xffff);
			}
		} else {
			pb = (int8_t*)num;
			for(i = 0;i < 16;i++) {
				if(i == 8) printf("  ");
				printf("%02x", (uint32_t)(*pb++) & 0xff);
			}
			pb = (int8_t*)num;
			for(i = 0;i < 16;i++) {
				c = *pb++;
				c = isprint(c) ? c : '.';
				printf("%c", c);
			}
		}
		putchar('\n');
		num += 16;
		for(w = 0;w < 100000;w++);
	}
	return NONE;
}

static uint32_t disk_write(uint32_t argc, STRING *argv) {
	int32_t	fd, size, addr, i, num;

	if(argc <= 4) return SYNTAX;
	sscanf(argv[2], "%x", &addr);
	sscanf(argv[3], "%x", &num);
	if(addr == -1 || num == -1 || argc != (num + 4) || num > 32) return SYNTAX;
	if((addr + num) > size) return IOERR;  // AT 2004/08/25
	{
		Byte	buf[num];
		int32_t	data;

		for(i = 0;i < num;i++) {
			sscanf(argv[i + 4], "%x", &data);
			buf[i] = (Byte)data;
		}
		writedata(0, filefd, addr, buf, num);
	}
	putchar('@');
	return NONE;
}

static uint32_t dir(uint32_t argc, STRING *argv) {
	int32_t	ret, fd;
	Entry	entry;

	for(fd = 0;;fd++) {
		ret = get_entry(0, fd, &entry);
		if(ret == -1) break;
		if(entry.name[0] == 0xff || entry.name[0] == 0) continue;
		printf("%-10s\t%6dbyte\r\n", entry.name, entry.size);
	}
	return NONE;
}

static uint32_t exec(uint32_t argc, STRING *argv) {
	return COMMAND;
}

static uint32_t newfile(uint32_t argc, STRING *argv) {
	uint32_t	num, ret, f;

	sscanf(argv[2], "%x", &num);
	f = new_fat(0, (num - 1) / disk[0].pagesize + 1);
	if(f == -1) return DSKFULL;
	del_entry(0, argv[1]);
	filefd = new_entry(0, argv[1], num, f);
	if(filefd == FREE) return DSKFULL;
	printf("file created.\n");
	return NONE;
}

static uint32_t del_file(uint32_t argc, STRING *argv) {
	uint32_t	ret;

	ret = del_entry(0, argv[1]);
	if(ret == -1) return NOFILE;
	printf("file deleted.\n");
	return NONE;
}

static uint32_t init() {
	uint32_t	adr;

        for(adr=0x30000;adr<0x80000;adr+=0x10000) {
		ioctl_device(disk[0].fd, adr, FLUSH_ERASE);
	}
	printf(" initialized.\n");
	return NONE;
}

static uint32_t set_data(uint32_t argc, STRING *argv) {
	int32_t	i, num, data;
	int8_t	*addrbyte;
	int16_t	*addrword;

	sscanf(argv[argc - 2], "%x", &data);
	addrbyte = (int8_t*)data;
	if(argc == 4 && argv[1][0] == 'w' && data & 1) return ARIGN; 
	addrword = (int16_t*)data;
	sscanf(argv[argc - 1], "%x", &num);
	if(addrbyte == (int8_t*)-1 || num == -1) return SYNTAX;
	if(argc == 4 && argv[1][0] == 'w') addrword[0] = num;
	else addrbyte[0] = num;
	printf(" updated.\n");
	return NONE;
}

static uint32_t clear_memory(uint32_t argc, STRING *argv) {
	int32_t	num, data;
	STRING	addr;

	sscanf(argv[1], "%x", &data);
	addr = (STRING)data;
	sscanf(argv[2], "%x", &num);
	if(addr == (STRING)-1 || num == -1)  return SYNTAX;
	bzero(addr, num);
	printf(" cleared.\n");
	return NONE;
}

static uint32_t echo(uint32_t argc, STRING *argv) {
	if(strcmp(argv[1], "off") == 0) echo_flag = 0;
	else if(strcmp(argv[1], "on") == 0) echo_flag = 1;
	else return SYNTAX;
	return NONE;
}

typedef struct {
	uint32_t	(*function)(uint32_t , STRING*);
	Byte		command[9];
	uint8_t		min, max, opt;
} function_table;

int main() {
	static const function_table table[] = {
		{dump,		"dump",	    2, 3, NONE},
		{set_data,	"setdata",  3, 4, NONE},
		{clear_memory,	"zeromem",  3, 3, NONE},
		{disk_write,	"eepdata",  1, MAX_ARG, FSCHK},
		{dir, 		"dir",      1, 1, FSCHK},
		{del_file,	"del",      2, 2, FSCHK},
		{newfile,	"newfile",  3, 3, FSCHK},
		{init,		"init",	    1, 1, FSCHK},
		{echo,          "echo",     2, 2, NONE},
		{exec, 		"",         1, MAX_ARG, NONE}
	};
	static Byte	buffer[MAX_BUF], *argv[MAX_ARG], data[16];
	uint32_t	ret, i, argc, fd, size, w;

	echo_flag = 1;
	int_enable();
	stdio = open_device(SuperPID, "sci1", 0);
	fsinit();
	fsready = 1;
	write_device(stdio, "\nCore OS 0.1s\n", 14);  // AT 2004/08/25
	strcpy(prompt, "\nCoreOS >"); // AT 2004/08/25
	for(;;) {
		printf(prompt);
		argc = read_line(buffer, argv);
		putchar('\n');
		if(argc >= MAX_ARG) {
			err_message(CMDFULL);
			continue;
		}
		for(i = 0;;i++) {
			if(strcmp(argv[0], table[i].command) == 0 || table[i].command[0] == 0) {
				if(argc >= table[i].min && argc <= table[i].max &&
				   ((table[i].opt == FSCHK && fsready > 0) ||
				    (table[i].opt == NONE))) {
					ret = (*(table[i].function))(argc, argv);
					if(ret > NONE) err_message(ret);
				} else {
					err_message(SYNTAX);
				}
				break;
			}
			if(table[i].command[0] == 0) break;
		}
	}
}
