void init_lcd();
int32_t open_lcd(uint16_t, int32_t, int32_t, int32_t);
int32_t open_lcdS(uint16_t, int32_t, int32_t, int32_t);
int32_t close_lcd(uint16_t, int32_t);
int32_t write_lcd(int32_t, STRING, int32_t);
int32_t read_lcd(int32_t, STRING, int32_t);
int32_t seek_lcd(int32_t, int32_t);
int32_t ioctl_lcd(int32_t, int32_t, int32_t);
