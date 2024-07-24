// --------------------------------------
//  Modified By				
//     T.Mochizuki 04/03/09             
// -------------------------------------
#define	IPNUM	4

#define UDP_SOCK	0x4000
#define TCP_SOCK	0x8000
#define SOCK_MASK	0x3fff

typedef struct {
  uint32_t ip;
  int16_t  timeout;
  Byte	 mac[6];
} IP_MAC;

typedef struct {
	Byte	 dest[6]	__attribute__((packed));
	Byte	 src[6]		__attribute__((packed));
	uint16_t type		__attribute__((packed));
} Ether;
#define IP_TYPE		0x800
#define ARP_TYPE	0x806

typedef struct {
	uint16_t hard		__attribute__((packed));
	uint16_t proto		__attribute__((packed));
	Byte	 hardsize	__attribute__((packed));
	Byte	 protosize	__attribute__((packed));
	uint16_t op		__attribute__((packed));
	Byte	 srcmac[6]	__attribute__((packed));
	uint32_t srcip		__attribute__((packed));
	Byte	 destmac[6]	__attribute__((packed));
	uint32_t destip		__attribute__((packed));
} Arp;
#define ETHER_TYPE	1
#define IEEE802_TYPE	6
#define	ARP_REQ		1
#define	ARP_ANS		2
typedef struct {
	Bits	 version     :4 __attribute__((packed));
	Bits	 header_len  :4 __attribute__((packed));
	Byte	 service	__attribute__((packed));
	uint16_t datasize	__attribute__((packed));
	uint16_t identifer	__attribute__((packed));
	Bits		     :1 __attribute__((packed));
	Bits	 div_enable  :1 __attribute__((packed));
	Bits	 end_packet  :1 __attribute__((packed));
	uint16_t offset	    :13 __attribute__((packed));
	Byte	 ttl		__attribute__((packed));
	Byte	 proto		__attribute__((packed));
	uint16_t checksum	__attribute__((packed));
	uint32_t srcip		__attribute__((packed));
	uint32_t destip		__attribute__((packed));
} IP_HDR;
#define ICMP	1
#define IPV4	4
#define TCP	6
#define UDP	17

typedef struct {
	Byte	 type		__attribute__((packed));
	Byte	 code		__attribute__((packed));
	uint16_t checksum	__attribute__((packed));
	uint16_t id		__attribute__((packed));
	uint16_t seq		__attribute__((packed));
} ICMP_HDR;
#define ICMP_ANS	0
#define ICMP_REQ	8

typedef struct {
	uint16_t src_port	__attribute__((packed));
	uint16_t dest_port	__attribute__((packed));
	uint16_t data_size	__attribute__((packed));
	uint16_t checksum	__attribute__((packed));
} UDP_HDR;

struct sockaddr {
	unsigned int	sin_addr;
	unsigned short	sin_port;
};

#define IPADDR(a,b,c,d) (unsigned int)a*0x1000000+b*0x10000+c*0x100+d

#define	TCP_FLAG_ACK	0x10
#define TCP_FLAG_PUSH	0x08
#define TCP_FLAG_RESET	0x04
#define TCP_FLAG_SYN	0x02
#define TCP_FLAG_FIN	0x01

#define TCP_INTFLAGS_CLOSEPENDING	0x01

#define	TCP_TYPE_NONE		0x00
#define	TCP_TYPE_SERVER		0x01
#define	TCP_TYPE_CLIENT		0x02
#define	TCP_TYPE_CLIENT_SERVER	0x03

#define	TCP_STATE_FREE		1
#define	TCP_STATE_RESERVED	2
#define	TCP_STATE_CLOSED	3
#define TCP_STATE_LISTENING	4
#define TCP_STATE_SYN_RECEIVED	5
#define	TCP_STATE_SYN_SENT	6
#define TCP_STATE_FINW1		7
#define	TCP_STATE_FINW2		8
#define TCP_STATE_CLOSING	9
#define	TCP_STATE_LAST_ACK	10
#define TCP_STATE_TIMED_WAIT	11
#define	TCP_STATE_CONNECTED	12

#define NO_OF_TCPSOCKETS	8
#define NO_OF_UDPSOCKETS	4
#define TCP_PORTS_END		1023
#define	MIN_TCP_HLEN		20
#define	MAX_TCP_OPTLEN		40
#define	TCP_DEF_RETRIES		7
#define	TCP_DEF_KEEPALIVE	4
#define TCP_DEF_RETRY_TOUT	2
#define TCP_TOS_NORMAL		0
#define TCP_DEF_TOUT		120
#define TCP_HALF_SEQ_SPACE	0x0000FFFF

#define TCP_EVENT_CONREQ	1
#define TCP_EVENT_CONNECTED	2
#define TCP_EVENT_CLOSE		4
#define TCP_EVENT_ABORT		8
#define TCP_EVENT_ACK		16
#define TCP_EVENT_REGENERATE	32
#define	TCP_EVENT_DATA		64
#define TCP_APP_OFFSET		MIN_TCP_HLEN

typedef struct {
	uint16_t sport	__attribute__((packed));
	uint16_t dport	__attribute__((packed));
	uint32_t seqno	__attribute__((packed));
	uint32_t ackno	__attribute__((packed));
	uint16_t hlen_flags	__attribute__((packed));
	uint16_t window	__attribute__((packed));
	uint16_t checksum	__attribute__((packed));
	uint16_t urgent	__attribute__((packed));
} TCP_HDR;
