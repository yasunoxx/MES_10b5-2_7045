void init_sci();
int32_t open_sci(uint16_t, int32_t, int32_t, int32_t);
int32_t close_sci(uint16_t, int32_t);
int32_t write_sci(int32_t, STRING, int32_t);
int32_t read_sci(int32_t, STRING, int32_t);
int32_t seek_sci(int32_t, int32_t);
int32_t ioctl_sci(int32_t, int32_t, int32_t);
