/************************/
/* functions of alloc.c */
/************************/
void	alloc_init(MemInfo*, uint32_t, uint32_t, STRING);
STRING	alloc_mem(MemInfo info, uint16_t id, uint32_t memsize);
int	free_mem(MemInfo info, uint16_t id, STRING);
void	free_idmem(MemInfo info, uint16_t id);
int	change_id_mem(MemInfo info, uint16_t oldid, uint16_t newid, STRING);

/************************/
/* functions of alloc.c */
/************************/
void	ring_init(RingInfo*, uint32_t, STRING);
int	write_ring(RingInfo*, STRING, uint32_t);
int	read_ring(RingInfo*, STRING, uint32_t);

/*************************/
/* functions of device.c */
/*************************/
void	init_device();
int32_t	open_device(uint16_t, STRING, uint32_t);
int32_t	close_device(uint16_t, uint32_t);
int32_t	write_device(uint32_t, STRING, uint32_t);
int32_t	read_device(uint32_t, STRING, uint32_t);
int32_t	seek_device(uint32_t, uint32_t);
int32_t	ioctl_device(uint32_t, uint32_t, uint32_t);

/*************************/
/* functions of coff.c */
/*************************/
int32_t stack_size(STRING);
int32_t get_program_size(STRING);
int32_t coff2bin(STRING, STRING, STRING[], void*[]);

/***********************/
/* functions of rofs.c */
/***********************/
int32_t rofs_new(int32_t, STRING, int32_t);
int32_t rofs_write(int32_t, int32_t, int32_t, STRING, int32_t);
int32_t rofs_find(int32_t, STRING, int32_t*);
int32_t rofs_del(int32_t, STRING);
int32_t rofs_read(int32_t, int32_t, int32_t, STRING, int32_t);
void	rofs_init(int32_t, uint32_t);

/***********************/
/* functions of que.c */
/***********************/
void init_que(QueInfo*, uint16_t, uint16_t, void*);
int32_t que_size(QueInfo*);
int32_t push_que(QueInfo*, void*);
int32_t add_que(QueInfo*, void*);
int32_t get_que(QueInfo*, uint16_t, void*, int32_t (*)(void*));
int32_t set_que(QueInfo*, uint16_t, void*);
int32_t del_que(QueInfo*, uint16_t);
int32_t search_que(QueInfo*, void*, int32_t (*)(void*, void*));
int32_t push_fifo(QueInfo*, void*);
int32_t pop_fifo(QueInfo*, void*);
