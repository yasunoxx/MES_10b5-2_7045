void	init_se(void);
int32_t open_se(uint16_t, int32_t, int32_t, int32_t);
int32_t close_se(uint16_t, int32_t);
int32_t write_se(int32_t, STRING, int32_t);
int32_t read_se(int32_t, STRING, int32_t);
int32_t seek_se(int32_t, int32_t);
int32_t ioctl_se(int32_t, int32_t, int32_t);

typedef struct {
	int16_t	id, fd;
	int32_t	size, page;
	int32_t	addr;
} SeepInfo;
