#include <string.h>
#include "../sys.h"
#define MAX_BUF	512
#define MAX_ARG	128
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
static char	prompt[16], echo_flag;
static char	cmd_buf[MAX_BUF], *cmd_argv[MAX_ARG];

static unsigned int read_line(char* buffer, char** argv) {
	unsigned int i, argc;
	char*	 ptr;

	ptr = buffer;
	for(;;) {
		if((unsigned int)ptr >= (unsigned int)(buffer + MAX_BUF)) ptr = &buffer[MAX_BUF - 1];
		*ptr = getchar();
		if(*ptr >= 0x80) continue;
		if(*ptr == '\r') break;
		if(*ptr == 0x7f) *ptr = 8;
		if(*ptr == 8) {
			if((unsigned int)ptr > (unsigned int)buffer) {
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
			if(argc >= (MAX_ARG - 1)) break;
			argv[argc++] = &(buffer[i + 1]);
		}
		i++;
	}
	argv[argc] = 0;
	return argc;
}

static void err_message(unsigned int id) {
	static const char message[10][20] = {
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
	unsigned int	i;

	i = (id > 9) ? 9 : id - 1;
	printf("ERROR[%s]\n", message[i]);
}

static unsigned int dump(unsigned int argc, char** argv) {
	unsigned int	  i, n, num;
	volatile unsigned int w;
	char		  *pb;
	short		  *pw;

	sscanf(argv[argc - 1], "%x", &num);
	n = num + 16 * 8;
	while(num < n) {
		printf("%08x : ", num);
		if(argc == 3 && argv[1][0] == 'w') {
			if(num & 1) return ARIGN;
			pw = (short*)num;
			for(i = 0;i < 8;i++) {
				if(i == 4) printf("  ");
				printf("%04x", (unsigned int)(*pw++) & 0xffff);
			}
		} else {
			pb = (char*)num;
			for(i = 0;i < 16;i++) {
				if(i == 8) printf("  ");
				printf("%02x", (unsigned int)(*pb++) & 0xff);
			}
		}
		putchar('\n');
		num += 16;
	}
	return NONE;
}

static unsigned int exec(unsigned int argc, char** argv) {
	int	pid, waitflag;

	waitflag = 1;
	if(argv[argc - 1][0] == '&' && argv[argc - 1][1] == 0) {
		waitflag = 0;
		argv[--argc] = 0;
	}
	pid = execute(argc, argv);
	if(pid == -1) return NOFILE;
	if(waitflag == 1) wait(pid);
	return NONE;
}

static unsigned int set_data(unsigned int argc, char** argv) {
	int	i, num, data;
	char	*addrchar;
	short	*addrword;

	sscanf(argv[argc - 2], "%x", &data);
	addrchar = (char*)data;
	if(argc == 4 && argv[1][0] == 'w' && data & 1) return ARIGN; 
	addrword = (short*)data;
	sscanf(argv[argc - 1], "%x", &num);
	if(addrchar == (char*)-1 || num == -1) return SYNTAX;
	if(argc == 4 && argv[1][0] == 'w') addrword[0] = num;
	else addrchar[0] = num;
	printf(" updated.\n");
	return NONE;
}

static unsigned int clear_memory(unsigned int argc, char** argv) {
	int	num, data;
	char*	addr;

	sscanf(argv[1], "%x", &data);
	addr = (char*)data;
	sscanf(argv[2], "%x", &num);
	if(addr == (char*)-1 || num == -1)  return SYNTAX;
	bzero(addr, num);
	printf(" cleared.\n");
	return NONE;
}

static unsigned int dir(unsigned int argc, char** argv) {
	int	size, i;
	char	name[64];

	for(i = 0;;i++) {
		size = getdirent((argc == 1) ? "" : argv[1], i, name, 63);
		if(size < 0) break;
		printf("%-10s\t%5d[char]\n", name, size);
	}
	return NONE;
}

static unsigned int newfile(unsigned int argc, char** argv) {
	unsigned int	fd;

	fd = open(argv[1], 0);
	if(fd == -1) return DSKFULL;
	fd = close(fd);
	printf("file created.\n");
	return NONE;
}

static unsigned int del_file(unsigned int argc, char** argv) {
	unsigned int	ret;

	ret = delete(argv[1]);
	if(ret == -1) return NOFILE;
	printf("file deleted.\n");
	return NONE;
}

static unsigned int pr_curdir(unsigned int argc, char** argv) {
	printf("%s\n", pwd());
	return NONE;
}

static unsigned int change_dir(unsigned int argc, char** argv) {
	unsigned int	ret;
	
	ret = cd(argv[1]);
	if(ret == -1) return NOFILE;
	return NONE;
}

static unsigned int disk_write(unsigned int argc, char** argv) {
	int	fd, addr, i, num;

	if(argc <= 4) return SYNTAX;
	sscanf(argv[2], "%x", &addr);
	sscanf(argv[3], "%x", &num);
	if(addr == -1 || num == -1 || argc != num + 4 || num > 32) return SYNTAX;
	fd = open(argv[1], APPEND_FILE);
	if(fd == -1) return NOFILE;
	{
		char	buf[num];
		int	data;

		for(i = 0;i < num;i++) {
			sscanf(argv[i + 4], "%x", &data);
			buf[i] = (char)data;
		}
		write(fd, buf, num);
	}
	close(fd);
	putchar('@');
	return NONE;
}

static unsigned int disk_read(unsigned int argc, char** argv) {
	int	fd, size, addr, i, num;

	sscanf(argv[2], "%x", &addr);
	sscanf(argv[3], "%x", &num);
	if(addr == -1 || num == -1) return SYNTAX;
	fd = open(argv[1], READ_FILE);
	if(fd == -1) return NOFILE;
	{
		char	buf[num];
		int	data;

		seek(fd, addr);
		read(fd, buf, num);
		for(i = 0;i < num;i++) {
			data = (int)buf[i] & 0xff;
			printf("%02x ", data);
		}
	}
	close(fd);
	putchar('@');
	return NONE;
}

static unsigned int disk_format(unsigned int argc, char** argv) {
	int	ret;

	ret = format(argv[1]);
	return (ret == -1) ? IOERR : NONE;
}

static void if_info(unsigned int num) {
	int	i, ip, mask, n, ret;
	char	buf[6];

	ret = getipinfo(num, &ip, &mask, buf);
	if(ret == -1) return;
	printf("net%d\tIP Address:", num);
	for(i = 0;i < 4;i++) {
		n = (ip >> ((3 - i) * 8)) & 0xff;
		printf("%d.", n);
	}
	printf(" Netmask:");
	for(i = 0;i < 4;i++) {
		n = (mask >> ((3 - i) * 8)) & 0xff;
		printf("%d.", n);
	}
	printf(" MAC:");
	for(i = 0;i < 5;i++) printf("%02x:", (unsigned int)buf[i] & 0xff);
	printf("%02x\n\n", (unsigned int)buf[i] & 0xff);
	ip = ifgateway(0);
	if(ip) {
		printf(" Gateway:");
		for(i = 0;i < 4;i++) {
			n = (ip >> ((3 - i) * 8)) & 0xff;
			printf("%d.", n);
		}
		printf("\n\n");
	}
}

static unsigned int strtoip(char *ptr, int *error) {
	int		i, j, n;
	unsigned int	ip;
	char		buf[4];

	*error = 0;
	for(i = 0;i < 4;i++) {
		for(j = 0;j < 3;j++) {
			if(!isdigit(ptr[j])) {
				*error = -1;
				return 0;
			}
			buf[j] = ptr[j];
			if(ptr[j + 1] == '.' || (ptr[j + 1] <= ' ' && i == 3)) {
				buf[j + 1] = 0;
				break;
			}
		}
		if(j == 3) {
			*error = -1;
			return 0;
		}
		sscanf(buf, "%d", &n);
		ip <<= 8;
		ip |= n;
		ptr = &(ptr[j + 2]);
	}
	return ip;
}

static unsigned int if_config(unsigned int argc, char** argv) {
	int	i, ip, mask, n, ret;

	if(argc == 2) return SYNTAX;
	if(argc == 1) {
		for(i = 0;i < 4;i++) if_info(i);
		return NONE;
	}
	if(memcmp(argv[2], "down", 4) == 0) {
		n = ifdown(argv[1]);
		if(n == -1) return IOERR;
		return NONE;
	}
	ip = strtoip(argv[2], &ret);
	if(ret == -1) return SYNTAX;
	n = ifconfig(argv[1], ip, 0xffffff00);
	if(n == -1) return IOERR;
	if_info(n);
	return NONE;
}

static unsigned int gateway(unsigned int argc, char** argv) {
	int	i, ip, mask, n, ret;

	if(argc != 2) return SYNTAX;
	ip = strtoip(argv[1], &ret);
	if(ret == -1) return SYNTAX;
	ifgateway(ip);
	return NONE;
}

static unsigned int dhcp(unsigned int argc, char** argv) {
	int	ret;
	char*	arg[2];

        ifgateway(0);
        ifdown(argv[1]);
	ret = if_dhcp(argv[1]);
	if(ret == 0) return IOERR;
	printf("Server is ");
	printf("%d.", (ret >> 24) & 0xff);
	printf("%d.", (ret >> 16) & 0xff);
	printf("%d.", (ret >> 8) & 0xff);
	printf("%d\n", ret & 0xff);
	arg[0] = "ifconfig";
	arg[1] = 0;
	if_config(1, arg);
	return NONE;
}

static unsigned int tftp(unsigned int argc, char** argv) {
	unsigned int	ip, ret, cmd_argc;

	ip = strtoip(argv[1], &ret);
	if(ret == -1) return SYNTAX;
	strcpy(prompt, "\ntftp >");
	for(;;) {
		printf(prompt);
		cmd_argc = read_line(cmd_buf, cmd_argv);
		putchar('\n');
		if(strcmp(cmd_argv[0], "quit") == 0) {
			break;
		} else if(strcmp(cmd_argv[0], "get") == 0 && cmd_argc == 2) {
			ret = tftp_load(0, ip, cmd_argv[1]);
			if(ret == -1) err_message(NOFILE);
		} else if(strcmp(cmd_argv[0], "put") == 0 && cmd_argc == 2) {
			ret = tftp_save(0, ip, cmd_argv[1]);
			if(ret == -1) err_message(NOFILE);
		} else err_message(SYNTAX);
	}
	strcpy(prompt, "\nMES >");
	return NONE;
}

static unsigned int echo(unsigned int argc, char** argv) {
	if(strcmp(argv[1], "off") == 0) echo_flag = 0;
	else if(strcmp(argv[1], "on") == 0) echo_flag = 1;
	else return SYNTAX;
	return NONE;
}

typedef struct {
	unsigned int	(*function)(unsigned int, char**);
	char		command[9];
	unsigned char	min, max, opt;
} function_table;

int main() {
	static const function_table table[] = {
		{dump,		"dump",	    2, 3, NONE},
		{set_data,	"setdata",  3, 4, NONE},
		{dir, 		"dir",      1, 2, NONE},
		{pr_curdir, 	"pwd",      1, 1, NONE},
		{change_dir, 	"cd",       2, 2, NONE},
		{clear_memory,	"zeromem",  3, 3, NONE},
		{if_config,	"ifconfig", 1, 3, NONE},
		{gateway,	"gateway",  2, 2, NONE},
		{dhcp,		"dhcp",	    2, 2, NONE},
		{tftp,		"tftp",	    2, 2, NONE},
		{disk_write,	"eepdata",  1, MAX_ARG, NONE},
		{disk_read,	"eepdump",  4, 4, NONE},
		{disk_format,	"format",   2, 2, NONE},
		{newfile,	"newfile",  3, 3, NONE},
		{del_file,	"del",      2, 2, NONE},
		{echo,		"echo",     2, 2, NONE},
		{exec, 		"",         1, MAX_ARG, NONE}
	};
	unsigned int	ret, i, argc, fsfd, fd, size;

	echo_flag = 1;
	printf("Micro Embeded System 1.0b5 special 2005/02/08\n");

	if_dhcp("ne0");
/*
        ifconfig("ne0",IPADDR(192,168,1,168), IPADDR(255,255,255,0));
        ifgateway(IPADDR(192,168,1,1));
*/

	strcpy(prompt, "\nMES >");
	for(;;) {
		printf(prompt);
		argc = read_line(cmd_buf, cmd_argv);
		putchar('\n');
		if(argc >= MAX_ARG) {
			err_message(CMDFULL);
			continue;
		}
		for(i = 0;;i++) {
			if(strcmp(cmd_argv[0], table[i].command) == 0 || table[i].command[0] == 0) {
				if(argc >= table[i].min && argc <= table[i].max) {
					ret = (*(table[i].function))(argc, cmd_argv);
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




