#include <string.h>
#include "../sys.h"

#define TFTP_SIZE	512

#define	TFTP_NOFILE	0
#define	TFTP_ACCESS	2
#define	TFTP_ILLEGAL	4

#define	TFTP_READREQ	1
#define	TFTP_WRITEREQ	2
#define	TFTP_DATA	3
#define TFTP_ACK	4
#define TFTP_ERROR	5

static int make_req_packet(char *packet, char *name, char command) {
	int	index;
	short	*data;

	index = 0;
	data = (short*)&packet[index];
	data[0] = command;
	index += sizeof(short);
	strcpy(&packet[index], name);
	index += strlen(name) + 1;
	strcpy(&packet[index], "octet");
	index += strlen("octet") + 1;
	return index;
}

static int ack_get(int sock,struct sockaddr *target) {
	unsigned short ack[2];

	ack[0] = 0;
	recvfrom(sock, (char*)ack, 4, target);
	if(ack[0] == TFTP_ACK) return ack[1];  // block number
	return -1;
}

static void ack_put(int sock,struct sockaddr *target, unsigned short pnum) {
	unsigned short ack[2];

	ack[0] = TFTP_ACK;
	ack[1] = pnum;  // block number
	sendto(sock, (char*)&ack, 4, target);
}

static int tftp_common(int netnum, int ip, char *filename, char command) {
	int		ack, i, c, sock, size, filesize, fd, ret, mode, fullsize;
	unsigned int	myip, servip, mask;
	unsigned char	mac[6];
	unsigned char	*packet;
	unsigned short	*data, pnum;
	struct sockaddr	target, myaddr;

	switch(command) {
	case TFTP_WRITEREQ:
		mode = READ_FILE;
		fullsize = TFTP_SIZE;
		break;
	case TFTP_READREQ:
		mode = 0;
		fullsize = TFTP_SIZE + 4;
		break;
	default:
		return -1;
	}
	ret = getipinfo(netnum, &myip, &mask, mac);
	if(ret == -1) return -1;
	fd = open(filename, mode);
	if(fd == -1) return -1;
	servip = ip;
	packet = malloc(TFTP_SIZE + 4);
	if(packet == 0) return -1;
	sock = udp_socket();
	bzero(packet, TFTP_SIZE + 4);

	target.sin_addr = servip;
	target.sin_port = 69;
	myaddr.sin_addr = myip;
	for(myaddr.sin_port = 1025;udp_bind(sock, &myaddr) == -1;myaddr.sin_port++);
	size = make_req_packet(packet, filename, command);
	data = (short*)packet;
	sendto(sock, packet, size, &target);
	ret = -1;
        if(command == TFTP_READREQ || ack_get(sock, &target) == 0) {
        	ret = 0;
	        pnum = 1;
		do {
			bzero(packet, TFTP_SIZE + 4);
			switch(command) {
			case TFTP_WRITEREQ:
				size = read(fd, &packet[4], TFTP_SIZE);
				for(i = 0;i < 4;i++) {
					if(command == TFTP_WRITEREQ) {
						data[0] = TFTP_DATA;
						data[1] = pnum;
						sendto(sock, packet, size + 4, &target);
						ack = ack_get(sock, &target);
						if(ack == -1) ret = -1;
						if(ack == pnum || ack == -1) break;
					}
				}
				if(i == 4) {
					ret = -1;
					break;
				}
				break;
			case TFTP_READREQ:
				size = recvfrom(sock, packet, TFTP_SIZE + 4, &target);
				write(fd, &packet[4], size - 4);
				data[0] = TFTP_ACK;
				sendto(sock, packet, 4, &target);
				break;
			}
			pnum++;
		} while(size == fullsize && ret == 0);
	}
	close(fd);
	udp_free(sock);
	free(packet);
	return ret;
}

int tftp_save(int netnum, int ip, char *filename) {
	return tftp_common(netnum, ip, filename, TFTP_WRITEREQ);
}

int tftp_load(int netnum, int ip, char *filename) {
	return tftp_common(netnum, ip, filename, TFTP_READREQ);
}
