#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

struct regsym {
	char		name[8];
	unsigned int	addr;
};

static struct regsym reg7045[] = {
	{"PAIOR", 0xfffff720},
	{"PACRH", 0xfffff722},
	{"PACRL", 0xfffff724},
	{"PHIOR", 0xfffff728},
	{"PHCR", 0xfffff72a},
	{"PBIOR", 0xfffff730},
	{"PBCRH", 0xfffff732},
	{"PBCRL", 0xfffff734},
	{"PBIR", 0xfffff736},
	{"PCIOR", 0xfffff73a},
	{"PCCR", 0xfffff73c},
	{"PCDR", 0xfffff73e},
	{"PDIOR", 0xfffff740},
	{"PDCRH", 0xfffff742},
	{"PDCRL", 0xfffff744},
	{"PFIOR", 0xfffff748},
	{"PFCRH", 0xfffff74a},
	{"PFCRL", 0xfffff74c},
	{"PEIOR", 0xfffff750},
	{"PECR", 0xfffff752},
	{"PLIOR", 0xfffff756},
	{"PLCRH", 0xfffff758},
	{"PLCRL", 0xfffff75a},
	{"PLIR", 0xfffff75c},
	{"PGIOR", 0xfffff760},
	{"PGCR", 0xfffff762},
	{"PJIOR", 0xfffff766},
	{"PJCRH", 0xfffff768},
	{"PJCRL", 0xfffff76a},
	{"PKIOR", 0xfffff770},
	{"PKCRH", 0xfffff772},
	{"PKCRL", 0xfffff774},
	{"PKIR", 0xfffff776},
	{"IPRA", 0xffffed00},
	{"IPRB", 0xffffed02},
	{"IPRC", 0xffffed04},
	{"IPRD", 0xffffed06},
	{"IPRE", 0xffffed08},
	{"IPRF", 0xffffed0a},
	{"IPRG", 0xffffed0c},
	{"IPRH", 0xffffed0e},
	{"IPRI", 0xffffed10},
	{"IPRJ", 0xffffed12},
	{"IPRK", 0xffffed14},
	{"IPRL", 0xffffed16},
	{"end",   0}
};

void err() {
	fprintf(stderr, "Syntax Error!!\n");
	exit(-1);
}

int get_symaddr(char *regname, struct regsym *list) {
	int	i;

	for(i = 0;memcmp(list[i].name, "end", 3) != 0;i++) {
		if(strcmp(regname, list[i].name) == 0) {
			return list[i].addr;
		}
	}
	sscanf(regname, "%x", &i);
	return i;
}

int main() {
	char	buffer[64], name[7], symname[10];
	unsigned char sum;
	int	i, bus, cs, cs_addr, addr, irq, rate, bits, b, n;

	printf( "S00A0000434F4E464947003F\n" );	// S-record header "CONFIG"

	addr = 0x400;
	while(!feof(stdin)) {
		scanf("%s", buffer);
		if(memcmp(buffer, "end", 3) == 0) break;
		buffer[6] = 0;
		bzero(name, 7);
		strcpy(name, buffer);
		scanf("%s", buffer);
		if(memcmp(buffer, "low=", 4) == 0) {
			sscanf(buffer + 4, "%d", &bus);
		} else if(memcmp(buffer, "sda=", 4) == 0) {
			sscanf(buffer + 4, "%d", &i);
			bus = 1 << i;
		} else {
			sscanf(buffer, "%d", &bus);
			if(bus > 8 && bus != 16 && bus != 32) err();
		}
		scanf("%s", buffer);
		if(memcmp(buffer, "NA", 3) == 0) {
			cs = 0xff;
		} else if(memcmp(buffer, "cs", 2) == 0 && isdigit(buffer[2])) {
			cs = buffer[2] - '0';
		} else if(memcmp(buffer, "col=", 4) == 0) {
			sscanf(buffer + 4, "%d", &cs);
		} else if(memcmp(buffer, "scl=", 4) == 0) {
			sscanf(buffer + 4, "%d", &i);
			cs = 1 << i;
		} else {
			err();
		}
		if(memcmp(name, "io7045", 6) == 0) {
			bzero(&name[2], 4);
			scanf("%s", symname);
			cs_addr = get_symaddr(symname, reg7045);
		} else {
			scanf("%x", &cs_addr);
		}
		bits = 0;
		if(memcmp(name, "lcd", 3) == 0) {
			n = (name[3] == 'S') ? 8 : 6;
			for(i = 0;i < n;i++) {
				scanf("%d", &b);
				b &= 0xf;
				bits <<= 4;
				bits |= b;
			}
		} else {
			scanf("%s", buffer);
			if(memcmp(buffer, "NA", 3) == 0) {
				irq = 0xff;
			} else if(memcmp(buffer, "irq", 3) == 0 && isdigit(buffer[3])) {
				irq = buffer[3] - '0';
			} else if(buffer[0] == 'f' && memcmp(name, "sci", 3) == 0) {
				sscanf(&buffer[1], "%d", &irq);
				if(irq == 0 || irq > 1000) err();
				rate = 0;
				for(i = 20;i >= 0;i -= 4) {
					rate *= 10;
					rate += (cs_addr >> i) & 0xf;
				}
				cs_addr = rate;
			} else {
				sscanf(buffer, "%x", &bits);
			}
		}
		if(bits == 0) printf("S110%04X", addr);
		else printf("S113%04X", addr);
		sum = 0x10;
		sum += (addr >> 8) & 0xff;
		sum += addr & 0xff;
		for(i = 0;i < 6;i++) {
			printf("%02X", name[i]);
			sum += name[i];
		}
		printf("%02X", bus);
		sum += bus;
		printf("%02X", cs);
		sum += cs;
		printf("%08X", cs_addr);
		sum += (cs_addr >> 24) & 0xff;
		sum += (cs_addr >> 16) & 0xff;
		sum += (cs_addr >> 8) & 0xff;
		sum += cs_addr & 0xff;
		if(bits != 0) {
			printf("%08X", bits);
			sum += (bits >> 24) & 0xff;
			sum += (bits >> 16) & 0xff;
			sum += (bits >> 8) & 0xff;
			sum += bits & 0xff;
		} else {
			printf("%02X", irq);
			sum += irq;
		}
		sum = ~sum;
		printf("%02X\n", sum);
		addr += 16;
	}

	printf( "S9030000FC\n" );	// S-record termination
}
