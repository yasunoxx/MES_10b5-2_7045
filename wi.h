void init_wi(void);
int32_t open_wi(uint16_t, int32_t, int32_t, int32_t);
int32_t open_wa(uint16_t, int32_t, int32_t, int32_t);
int32_t close_wi(uint16_t, int32_t);
int32_t write_wi(int32_t, STRING, int32_t);
int32_t read_wi(int32_t, STRING, int32_t);
int32_t seek_wi(int32_t, int32_t);
int32_t ioctl_wi(int32_t, int32_t, int32_t);
