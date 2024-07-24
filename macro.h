typedef unsigned char	Byte;
typedef unsigned char	Bits;
typedef unsigned char *	STRING;
typedef char		int8_t;
typedef short		int16_t;
typedef	long		int32_t;
typedef	long long	int64_t;
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef	unsigned long	uint32_t;
typedef	unsigned long long uint64_t;
typedef unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef	unsigned long	u_int32_t;
typedef	unsigned long long u_int64_t;
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	void		*caddr_t;

#define	bswap16(x)	((((x) & 0xff00) >>  8) | (((x) & 0x00ff) <<  8))
#define	bswap32(x)	((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
			(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define	htobe16(x)	((uint16_t)(x))
#define	htobe32(x)	((uint32_t)(x))
#define	htole16(x)	bswap16((x))
#define	htole32(x)	bswap32((x))
#define	be16toh(x)	((uint16_t)(x))
#define	be32toh(x)	((uint32_t)(x))
#define	le16toh(x)	bswap16((x))
#define	le32toh(x)	bswap32((x))
