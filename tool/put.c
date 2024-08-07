#define  LINUX
#undef	FreeBSD
#undef	Solaris
#undef	Win32

///////////////////////////////////////
// H8/OS data tranfer to ram
//        Copyleft Yukio Mituiwa,2002
// License is GPL2
//      Update 2002,Dec,18 senshu@astro.yamagata-cit.ac.jp
//                                     addtional -g option
///////////////////////////////////////

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<fcntl.h>
#ifndef Win32
 #include	<unistd.h>
 #include	<strings.h>
 #include	<sys/param.h>
 #include	<libgen.h>
#endif

#ifdef Win32
 #define        RSLINE  "com1"
#endif
#ifdef LINUX
 #define	RSLINE	"/dev/ttyS0"
#endif
#ifdef FreeBSD
 #define	RSLINE	"/dev/cuaa0"
#endif
#ifdef Solaris
 #define	RSLINE	"/dev/cua/b"
#endif

/*
Newsgroups: mod.std.unix
Subject: public domain AT&T getopt source
Date: 3 Nov 85 19:34:15 GMT

Here's something you've all been waiting for:  the AT&T public domain
source for getopt(3).  It is the code which was given out at the 1985
UNIFORUM conference in Dallas.  I obtained it by electronic mail
directly from AT&T.  The people there assure me that it is indeed
in the public domain.
*/

#ifdef Win32
 #include <io.h>
#endif

#define ERR(s, c)	if(opterr){\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;
unsigned int exec_addr;

#ifdef Win32
int getopt(int argc, char **argv, char *opts)
{
    static int sp = 1;
    register int c;
    register char *cp;

    if (sp == 1)
	if (optind >= argc || argv[optind][0] != '-'
	    || argv[optind][1] == '\0') return (EOF);
	else if (strcmp(argv[optind], "--") == NULL) {
	    optind++;
	    return (EOF);
	}
    optopt = c = argv[optind][sp];
    if (c == ':' || (cp = strchr(opts, c)) == NULL) {
	ERR(": illegal option -- ", c);
	if (argv[optind][++sp] == '\0') {
	    optind++;
	    sp = 1;
	}
	return ('?');
    }
    if (*++cp == ':') {
	if (argv[optind][sp + 1] != '\0')
	    optarg = &argv[optind++][sp + 1];
	else if (++optind >= argc) {
	    ERR(": option requires an argument -- ", c);
	    sp = 1;
	    return ('?');
	} else
	    optarg = argv[optind++];
	sp = 1;
    } else {
	if (argv[optind][++sp] == '\0') {
	    sp = 1;
	    optind++;
	}
	optarg = NULL;
    }
    return (c);
}
#endif

unsigned char sPort[32];
char *exec_name = "h8put";

/* PROTO TYPES */
int send_mem_data(unsigned char *filename, int wait, int load_exec, int eeprom);
int send_exec(int wait, unsigned int exec);

#ifdef Win32
#include	<windows.h>
HANDLE TheFd;
OVERLAPPED WOp;
OVERLAPPED ROp;

int writecom(char *data, int size)
{
    unsigned long num;

    WriteFile(TheFd, data, size, &num, &WOp);
    return num;
}

int getbyte(void)
{
    char buf[2];
    int s;

    do {
	ReadFile(TheFd, buf, 1, &s, &ROp);
    }
    while (s < 1);
    return buf[0];
}
#else
#include <termios.h>
struct termios TheTty;
int TheFd;

int writecom(char *data, int size)
{
    int num;

    num = write(TheFd, data, size);
    return num;
}

int getbyte(void)
{
    char buf[2];
    int s;

    do {
	if ((s = read(TheFd, buf, 1)) == -1) {
	    fprintf(stderr, "read error\n");
	    exit(-1);
	}
    }
    while (s < 1);
    return buf[0];
}
#endif

void putbyte(int c)
{
    char buf[2];

    buf[0] = c;
    writecom(buf, 1);
}

int init(void)
{
#ifdef Win32
    DCB dcb;

    ZeroMemory(&WOp, sizeof(WOp));
    ZeroMemory(&ROp, sizeof(ROp));
    WOp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ROp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    TheFd = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE,
		       0, NULL, OPEN_EXISTING, 0, NULL);
    if (TheFd == INVALID_HANDLE_VALUE) {
	return -1;
    }
    dcb.DCBlength = sizeof(DCB);
    GetCommState(TheFd, &dcb);
    dcb.BaudRate = 57600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(TheFd, &dcb);
#else
    if ((TheFd = open(sPort, O_RDWR)) == -1) {
	return -1;
    }
    tcsetpgrp(TheFd, getpgrp());
    tcgetattr(TheFd, &TheTty);
    cfsetispeed(&TheTty, B57600);
    cfsetospeed(&TheTty, B57600);
    TheTty.c_cflag |= HUPCL | CLOCAL;
    TheTty.c_cflag &= ~(PARENB | CSTOPB);
    TheTty.c_lflag &= ~( ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK | ECHONL );
    TheTty.c_iflag &= ~( INPCK | ISTRIP);
    TheTty.c_oflag &= ~( OPOST );
    TheTty.c_cc[VTIME] = 0;
    TheTty.c_cc[VMIN] = 1;
    if (tcsetattr(TheFd, TCSANOW, &TheTty) == -1) {
	fprintf(stderr, "stty error\n");
	return -1;
    }
#endif
    return 0;
}

int get_mem_size(unsigned char *filename)
{
    FILE	*fp;
    int		i, addr, count, size;
    unsigned int minaddr, maxaddr;
    unsigned char combuf[512], numbuf[10], *ptr;

    maxaddr = 0;
    minaddr = 0x7fffffff;
    if ((fp = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "file (%s) not found!!\n", filename);
	return 2;
    }
    fgets(combuf, 255, fp);
    while (!feof(fp)) {
	if (combuf[0] != 'S') {
	    fgets(combuf, 255, fp);
	    continue;
	}
	numbuf[0] = combuf[1];
	numbuf[1] = 0;
	size = atoi(numbuf);
	if (size < 1 || size > 3) {
	    fgets(combuf, 255, fp);
	    continue;
	}
	numbuf[0] = combuf[2];
	numbuf[1] = combuf[3];
	numbuf[2] = 0;
	sscanf(numbuf, "%x", &count);
	ptr = combuf + 4;
	for (i = 0; i < size + 1; i++) {
	    numbuf[i * 2] = *(ptr++);
	    numbuf[i * 2 + 1] = *(ptr++);
	}
	numbuf[i * 2] = 0;
	sscanf(numbuf, "%x", &addr);
	if((addr + count - 3 - size) > maxaddr) maxaddr = addr + count - 3 - size;
	if(addr < minaddr) minaddr = addr;
	fgets(combuf, 255, fp);
    }
    fclose(fp);
    return maxaddr - minaddr + 1;
}

int send_mem_data(unsigned char *filename, int wait, int load_exec, int eeprom)
{
    FILE	*fp;
    int		n, i, c, addr, count, size, fail, curline, memsiz;
    unsigned char combuf[512];
    unsigned char orig[256], verify[256];
    unsigned char numbuf[9], *ptr, *rsp;
    unsigned char rsbuf[1024];
    unsigned char edit_buff[256];

    volatile int j;

    memsiz = get_mem_size(filename);
    curline = 0;
    if ((fp = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "file (%s) not found!!\n", filename);
	return 2;
    }
    putbyte('\r');
    fgets(combuf, 255, fp);
    c = getbyte();
    while (c != '>') {
	c = getbyte();
    }
    if(eeprom > 0) {
    	sprintf(rsbuf, "newfile %s %x\r", filename, memsiz);
    	n = strlen(rsbuf);
    	for (i = 0; i < n; i++) {
    	   for (j = 0; j < (wait * 10000); j++);
	   putbyte(rsbuf[i]);
	}
	fgets(combuf, 255, fp);
	c = getbyte();
	while (c != '>') {
	    c = getbyte();
	}
    }
    while (!feof(fp)) {
	if (combuf[0] != 'S') {
	    fgets(combuf, 255, fp);
	    continue;
	}
	numbuf[0] = combuf[1];
	numbuf[1] = 0;
	size = atoi(numbuf);
	if (size < 1 || size > 3) {
	    fgets(combuf, 255, fp);
	    continue;
	}
	numbuf[0] = combuf[2];
	numbuf[1] = combuf[3];
	numbuf[2] = 0;
	sscanf(numbuf, "%x", &count);
	ptr = combuf + 4;
	for (i = 0; i < size + 1; i++) {
	    numbuf[i * 2] = *(ptr++);
	    numbuf[i * 2 + 1] = *(ptr++);
	}
	numbuf[i * 2] = 0;
	sscanf(numbuf, "%x", &addr);
	if(load_exec != 0 && curline == 0) exec_addr = addr;
	if(eeprom == 0) {
	    sprintf(rsbuf, "setdata %x %x", addr, count - 2 - size);
	} else {
	    sprintf(rsbuf, "eepdata %x %x", addr, count - 2 - size);
	}
	rsp = &(rsbuf[strlen(rsbuf)]);
	for (i = 0; i < count - 2 - size; i++) {
	    *(rsp++) = ' ';
	    *(rsp++) = *(ptr++);
	    *(rsp++) = *(ptr++);
	}
	*(rsp++) = '\r';
	*rsp = 0;
	n = strlen(rsbuf);
	strcpy(orig, rsbuf);
	for (i = 0; i < n; i++) {
	    for (j = 0; j < (wait * 10000); j++);
	    putbyte(rsbuf[i]);
	}
	orig[n - 1] = 0;
	fail = 1;
	i = 0;
	c = getbyte();
	while (c != '>') {
	    if (c >= 0x20 && c < 0x80)
		verify[i++] = c;
	    if (c == '.') {
		fail = 0;
		verify[--i] = 0;
		i++;
	    }
	    if (c == '\r')
		verify[i++] = 0;
	    c = getbyte();
	}
	if (strcmp(orig, verify) != 0)
	    fail = 1;
	if (fail == 0) {
	    fgets(combuf, 255, fp);
#if 1
	    if (verify[16] == ' ') {
		strncpy(edit_buff, verify + 8, 7);
		edit_buff[7] = '0';
		strcpy(edit_buff + 8, verify + 15);
		edit_buff[8] = toupper(edit_buff[8]);
		puts(edit_buff);
	    } else {
		puts(verify + 8);
	    }
#else
	    puts(verify + 8);
#endif
	}
	curline++;
    }
    fclose(fp);

    return 0;
}

int send_exec(int wait, unsigned int exec)
{
    int n, i, j, c;
    unsigned char combuf[512];
    unsigned char rsbuf[1024];

    if (exec != 0) {
	putbyte('\r');
	c = getbyte();
	while (c != '>') {
	    c = getbyte();
	}
	sprintf(rsbuf, "exec %x\r", exec);
	n = strlen(rsbuf);
	for (i = 0; i < n; i++) {
	    for (j = 0; j < (wait * 10000); j++);
	    putbyte(rsbuf[i]);
	}
	printf("==== H8/OS Apps start ====\n");
	j = 0;
	while (1) {
	    c = getbyte();
	    j++;
	    if (n == j)
		putchar('\n');
	    putchar(c);
	}
    }
    return 0;
}

int main(int argc, char **argv)
{
    FILE *fp;
    int c, ch, retval;
    int test_mode, wait, exec_only, load_exec, eeprom;
    int this_option_optind = optind ? optind : 1;

    /* init */
    test_mode = exec_addr = retval = exec_only = load_exec = eeprom = 0;
    wait = 4;

    /* parse options */
    strcpy(sPort, RSLINE);

    if (argc == 1)
	goto Usage;
    while ((ch = getopt(argc, argv, "rteg:w:S:")) != EOF) {
	switch (ch) {
	case 'r':
	    eeprom++;
	    break;

	case 't':
	    test_mode++;
	    break;

	case 'e':
	    load_exec++;
	    break;

	case 'g':
	    exec_only++;
	    c = atoi(optarg);
	    switch (c) {
	    case 1:
		exec_addr = 0xfff520;
		break;

	    case 2:
		exec_addr = 0xfff600;
		break;

	    case 3:
		exec_addr = 0xffd940;
		break;

	    default:
		sscanf(optarg, "%x", &exec_addr);
		break;
	    }
	    printf("exec_addr = %06x\n", exec_addr);
	    break;

	case 'w':
	    wait = atoi(optarg);
	    break;
	case 'S':
	    strncpy(sPort, optarg, sizeof sPort);
	    break;

	case '?':
	default:
	  Usage:
	    fprintf(stderr,
    "Usage: %s [-r] [-w N] [-g {1|2|3|addr}] [-e] [-t] [-S Serial-port] mot-format\n",
		    exec_name);
	    fprintf(stderr, "   -t:  test mode.\n");
	    fprintf(stderr, "   -w:  wait param.\n");
	    fprintf(stderr, "   -e:  load & exec.\n");
	    fprintf(stderr, "   -g:  exec.\n");
	    fprintf(stderr, "   -r:  write to seeprom.\n");
	    fprintf(stderr, "Serial port is %s.\n", sPort);
	    break;
	}
    }

    if(eeprom > 0) {
    	load_exec = 0;
    	exec_addr = 0;
    }

    if (init() < 0) {
	printf("Serial port(%s) init error.\n", sPort);
	exit(3);
    }

#if 0
    printf("==== init end ====\n");
#endif
    if (test_mode) {
	printf("==== monitor mode start ====\n");
	while ((c = getbyte()) != '\r') {
	    putchar(c);
	}
	putchar('\n');
	printf("==== monitor mode end ====\n");
    } else {
	if (optind < argc) {
#ifdef TEST
	    printf("test_mode = %d, wait = %d, exec = %x\n",
		   test_mode, wait, exec_addr);
	    while (optind < argc)
		printf("%s ", argv[optind++]);
#else
	    if (exec_only) {
		retval = send_exec(wait, exec_addr);
	    } else {
		printf("==== send %s ====\n", argv[optind]);
		retval = send_mem_data(argv[optind], wait, load_exec, eeprom);
//		if (exec_addr != 0) 
//		    retval = send_exec(wait, exec_addr);
	    }
#endif
	} else {
	    if (exec_only) {
		retval = send_exec(wait, exec_addr);
	    }
	}
    }
  EXIT:
#ifdef Win32
    CloseHandle(TheFd);
#else
    close(TheFd);
#endif

    return retval;
}
