#include "../const.h"
#define read	s_read
#define write	s_write
#define open	s_open
#define close	s_close
#define seek	s_seek
#define ioctl	s_ioctl
#define exit	s_exit
#define wait	s_wait
#define malloc	s_malloc
#define free	s_free
#define kill	s_kill
#define pwd	s_pwd
#define cd	s_cd
#define format	s_format
#define printf __printf
#define fprintf __fprintf
#define sprintf __sprintf
#define scanf __scanf
#define fscanf __fscanf
#define sscanf __sscanf
#define fputc __fputc
#define fgetc __fgetc
#define putchar __putchar
#define getchar __getchar
#define IPADDR(a,b,c,d) (unsigned int)a*0x1000000+b*0x10000+c*0x100+d
struct sockaddr {
	unsigned int	sin_addr;
	unsigned short	sin_port;
};
struct Entry {
	unsigned long 	size;
	unsigned int	fd;
	unsigned short	drive;
	unsigned short	index;
	unsigned char	name[12];
};

char*	s_malloc(int);
int	s_free(char*);
void	s_exit(void);
int	if_dhcp(char*);
int	tftp_load(int, int, char*);
int	execute(int, char**);
int	exec_redirect(int, char**, int, int);
int	set_stdout(int, int);
int	set_stdin(int, int);
int	kill(int);
int	s_wait(int);
int	get_pid(void);
void	sleep(int);
int	ifconfig(char*, int, int);
int	ifdown(char*);
int	ifup(char*);
int	ifgateway(int);
int	getipinfo(int, int*, int*, char*);
int	getip(int);
int	get_mac_address(int, char*);
int	udp_socket(void);
int	udp_bind(int, struct sockaddr*);
int	udp_free(int);
int	recvfrom(int, char*, int, struct sockaddr*);
int	sendto(int, char*, int, struct sockaddr*);
int	tcp_socket(void);
int	tcp_free(int);
int	tcp_accept(int, struct sockaddr*);
int	tcp_bind(int, struct sockaddr*);
int	tcp_connect(int, struct sockaddr*);
int	tcp_recv(int, char*, int);
int	tcp_send(int, char*, int);
int	s_write(int, char*, int);
int	s_read(int, char*, int);
int	s_open(char*, int);
int	s_close(int);
int	s_seek(int, int);
int	s_ioctl(int, int, int);
void	set_timer(void (*)(), int);
int	delete(char*);
int	getdirent(char*, int, char*, int);
char	*s_pwd(void);
int	s_cd(char*);
int	cgi_value(char*, char*, char*, int);
char    *get_version();

