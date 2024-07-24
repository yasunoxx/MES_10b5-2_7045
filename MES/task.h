#define MAX_TASK	64
 #define PR_REG		0
 #define PC_REG		18
 #define ARGC		13
 #define ARGV		12
 #define STACK_NUM	-20
 #define TRAP0	asm("trapa #0x20")
 #define TRAP1	asm("trapa #0x21")
 #define TRAP2	asm("trapa #0x22")
 #define TCSR_RD (*(volatile unsigned char *)0xffffec10)
 #define TCNT_RD (*(volatile unsigned char *)0xffffec11)
 #define TCSR_WR (*(volatile unsigned short *)0xffffec10)
 #define TCNT_WR (*(volatile unsigned short *)0xffffec10)

#define SuperPID	1

#define	REQ_STATE	0x01
#define	EXIT_STATE	0x02
#define	WAIT_STATE	0x04
#define	NET_STATE	0x08

typedef struct TASK {
	int32_t		ppid, pid, wait_pid, state;
	int32_t		count, net_count, retry;
	int32_t		interval;
	void		(*sigfunc)();
	STRING		pwd;
	int32_t		*sp;
	struct TASK	*next, *prev;
	int32_t		req, retval, arg[4];
	int32_t		stdout, stdin, stderr;
} Task;

struct Entry {
	unsigned long	size;
	unsigned int	fd;
	unsigned short	drive;
	unsigned short	index;
	unsigned char	name[12];
};

#define	HEAPSIZE	256

#define	NONE_REQ	0
#define	OPEN_REQ	1
#define	CLOSE_REQ	2
#define	READ_REQ	3
#define	WRITE_REQ	4
#define	SEEK_REQ	5
#define	CONTROL_REQ	6
#define	DELETE_REQ	7
#define	EXEC_REQ	8
#define	KILL_REQ	9
#define	ALLOC_REQ	10
#define	FREE_REQ	11
#define	WAIT_REQ	12
#define	GETDIR_REQ	13
#define	STDIN_REQ	14
#define	STDOUT_REQ	15
#define	FORMAT_REQ	16

#define	IFCONFIG_REQ	20
#define	IPINFO_REQ	21
#define	IFDOWN_REQ	22
#define	IFUP_REQ	23
#define	GETIP_REQ	24
#define	IFGATEWAY_REQ	25
#define	SEND_ARP_REQ	26

#define	UDP_SOCK_REQ	30
#define	UDP_FREE_REQ	31
#define	UDP_BIND_REQ	32
#define	UDP_SEND_REQ	33
#define	UDP_RECV_REQ	34

#define	TCP_SOCK_REQ	40
#define	TCP_FREE_REQ	41
#define	TCP_LISTEN_REQ	42
#define	TCP_BIND_REQ	43
#define	TCP_CONN_REQ	44
#define	TCP_ACCEPT_REQ	45
#define	TCP_RECV_REQ	46
#define	TCP_SEND_REQ	47

#define	SHM_GET_REQ	50
#define	SHM_AT_REQ	51
#define	SHM_DT_REQ	52

#define STDOUT_FD	0
#define STDIN_FD	1
#define STDERR_FD	2
