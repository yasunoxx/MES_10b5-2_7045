#include "../sys.h"
#include <string.h>
#define OPTSIZ	1400
#define READSIZ	700
static unsigned char	op[8], dev[64], htmlname[32];
static int		port, csock;

void not_found(int sock, char *data, char *ver) {
	int	n;

	strcpy(data, ver);
	n = strlen(data);
	strcpy(data + n, " 404 Not Found\n");
	n = strlen(data);
	strcpy(data + n, "Server: MES(H8/OS 5.0)\n");
	n += strlen(data + n);
	strcpy(data + n, "Content-length: 46\n\n");
	n += strlen(data + n);
	strcpy(data + n, "<html><body><h1>Not Found!!</h1></body></html>");
	n += strlen(data + n);
	tcp_send(sock, data, n);
}

int parse(int sock, char *data) {
	static char	name[OPTSIZ];
	char	com[8], ver[16], tmpname[32];
	char	*ptr1, *ptr2, *p, *opt, *argv[3];
	int	i, n, fd, filesize, offset, size, pid, argc;

	ptr1 = data;
	ptr2 = strchr(data, ' ');
	if(ptr2 == 0) return -1;
	*ptr2 = 0;
	strcpy(com, ptr1);
	ptr1 = ptr2 + 1;
	ptr2 = strchr(ptr1, ' ');
	if(ptr2 == 0) return -1;
	*ptr2 = 0;
	name[0] = 0;
	if(ptr1[1] == 0 || ptr1[1] == '?') strcpy(name, htmlname);
	strncat(name, &ptr1[1], OPTSIZ - 10);
	if(name[0] == '~') name[0] = '/';
	p = strchr(name, '?');
	if(p == 0) {
		opt = &name[strlen(name)];
	} else {
		*p = 0;
		opt = p + 1;
	}
	ptr1 = ptr2 + 1;
	ptr2 = strchr(ptr1, '\r');
	if(ptr2 == 0) return -1;
	*ptr2 = 0;
	strcpy(ver, ptr1);
	if(strcmp(com, "GET") == 0) {
		tmpname[0] = 0;
		if(strchr(name, '.') == 0) {
			sprintf(tmpname, "/ram0/tmp%d", sock);
			fd = open(tmpname, 0);
			argc = 2;
			argv[0] = name;
			argv[1] = opt;
			argv[2] = 0;
			pid = exec_redirect(argc, argv, fd, 0);
			if(pid == -1) {
				close(fd);
				not_found(sock, data, ver);
				return -1;
			}
			wait(pid);
			close(fd);
			fd = open(tmpname, READ_FILE);
		} else {
			fd = open(name, READ_FILE);
			if(fd == -1) {
				not_found(sock, data, ver);
				return -1;
			}
		}
		filesize = ioctl(fd, 0, FILE_SIZE);
		strcpy(data, ver);
		n = strlen(data);
		strcpy(data + n, " 200 Document follows\n");
		n = strlen(data);
		strcpy(data + n, "Server: MES(H8/OS 5.0)\n");
		n += strlen(data + n);
		sprintf(data + n, "Content-length: %d\n\n", filesize);
		n += strlen(data + n);
		tcp_send(sock, data, n);
		for(i = 0;i < filesize;i += OPTSIZ) {
			size = ((filesize - i) < OPTSIZ) ? filesize - i : OPTSIZ;
			bzero(data, OPTSIZ);
			read(fd, data, size);
			tcp_send(sock, data, size);
		}
		close(fd);
		if(tmpname[0] != 0) delete(tmpname);
	}
	return 0;
}

int server(char *filename) {
	int		pid, ret, sock, dsock;
	char		*args[6], numchar[8];
	struct sockaddr myaddr, cliaddr;

	sock = tcp_socket();
	myaddr.sin_addr = getip(0);
	myaddr.sin_port = port;
	ret = tcp_listen(sock, &myaddr);
	if(ret == -1) {
		tcp_free(sock);
		printf("Already used.\n");
		return -1;
	}
	while(1) {
		dsock = tcp_accept(sock, &cliaddr);
		if(dsock < 0) continue;
		printf("Connect...");
		args[0] = filename;
		args[1] = "client";
		sprintf(numchar, "%d", dsock);
		args[2] = numchar;
		args[3] = dev;
		args[4] = htmlname;
		args[5] = 0;
		pid = execute(5, args);
	}
}

int client() {
	static char	data[OPTSIZ], buffer[READSIZ];
	char		*p;
	int		dsock, offset, remain, size;

	cd(dev);
	dsock = csock;
	for(;;) {
		offset = 0;
		remain = OPTSIZ;
		bzero(data, OPTSIZ);
		do {
			bzero(buffer, READSIZ);
			size = tcp_recv(dsock, buffer, READSIZ);
			if(size < 0) break;
			if(remain > 0) memcpy(&data[offset], buffer, (size > remain) ? remain : size);
			offset += size;
			remain -= size;
			p = strrchr(buffer, '\n');
		} while(p == 0 || (p[-1] != '\n' &&  p[-2] != '\n'));
		if(size < 0) break;
		data[offset] = 0;
		parse(dsock, data);
	}
	printf("Disconnect\n");
	tcp_free(dsock);
}

int main(int argc, char **argv) {
	int	i;
	char	*filename;

	strcpy(op, "server");
	strcpy(dev, "/rom0/");
	strcpy(htmlname, "/=index.html");
	port = 80;
	for(i = 1;i < argc;i++) {
		if(isdigit(argv[i][0])) {
			sscanf(argv[i], "%d", &port);
		} else if(argv[i][0] == '/') {
			if(argv[i][1] == '=') {
				strncpy(htmlname, &argv[i][2], 31);
			} else {
				strncpy(dev, argv[i], 63);
			}
		} else if(strcmp(argv[i], "server") == 0 || strcmp(argv[i], "client") == 0) {
			strcpy(op, argv[i]);
		} else {
			printf("Illegal option error!!\n");
			return -1;
		}
	}
	csock = port;
	if(op[0] == 's') server(argv[0]);
	else client();
	return 0;
}
