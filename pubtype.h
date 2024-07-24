/* for alloc.c */
typedef struct {
	uint32_t memsize;
	uint32_t secsize;
	STRING	 memarea;
	uint32_t secstart;
	uint32_t secend;
} MemInfo;

/* for ring.c */
typedef struct {
	uint32_t maxsize;
	uint32_t start;
	uint32_t datasize;
	STRING	 ptr;
} RingInfo;

/* for que.c */
typedef struct {
	uint16_t typesize;
	uint16_t datasize;
	uint16_t maxsize;
	uint16_t start;
	void	 *area;
} QueInfo;

/* for system configuration (1) */
typedef struct {
	Byte	 name[6];
	uint8_t	 bus;	
	uint8_t	 cs;	
	uint32_t address;	
	uint8_t	 irq;
	Byte	 reserve[3];	
} SysInfo;

/* for system configuration (2) */
typedef struct {
	Byte	 name[6];
	uint8_t	 bus;	
	uint8_t	 cs;	
	uint32_t address;	
	uint32_t size;	
} SysInfo2;

/* for LCD configuration */
typedef struct {
	Byte	 name[6];
	uint8_t	 low;	
	uint8_t	 column;
	uint32_t address;	
	uint32_t bits;
} LCDConf;
