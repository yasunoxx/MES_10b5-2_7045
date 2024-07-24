void init_ram();
int32_t open_ram(uint16_t, int32_t, int32_t, int32_t);
int32_t close_ram(uint16_t, int32_t);
int32_t write_ram(int32_t, STRING, int32_t);
int32_t read_ram(int32_t, STRING, int32_t);
int32_t seek_ram(int32_t, int32_t);
int32_t ioctl_ram(int32_t, int32_t, int32_t);

