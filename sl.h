void	init_sl811();
int32_t open_sl811(uint16_t, int32_t, int32_t, int32_t);
int32_t close_sl811(uint16_t, int32_t);
int32_t seek_sl811(int32_t, int32_t);
int32_t write_sl811(int32_t, STRING, int32_t);
int32_t read_sl811(int32_t, STRING, int32_t);
int32_t ioctl_sl811(int32_t, int32_t, int32_t);
