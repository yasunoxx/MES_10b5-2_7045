#define	FATSIZEOF	sizeof(uint16_t)
#define	CACHESIZ	2048
#define MAXPATHSIZ	128
#define FREE		0xffff
#define TAIL		0x7fff

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
	uint32_t info;
	uint16_t firstfat;
	uint16_t fd;
	Byte	 name[8];
} DiskInfo;

typedef struct {
	Entry	ent;
	Byte	*cache;
	int32_t	entindex;
	int32_t	offset;
	int32_t	areasize;
	int32_t	current;
	int32_t	id;
	int32_t	option;
	uint16_t root;
	uint16_t num;
} FdInfo;
