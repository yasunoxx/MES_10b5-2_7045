void init_rom();
int32_t open_rom(uint16_t, int32_t, int32_t, int32_t);
int32_t close_rom(uint16_t, int32_t);
int32_t write_rom(int32_t, STRING, int32_t);
int32_t read_rom(int32_t, STRING, int32_t);
int32_t seek_rom(int32_t, int32_t);
int32_t ioctl_rom(int32_t, int32_t, int32_t);
