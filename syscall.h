#include "macro.h"
#include "pubtype.h"
#include "const.h"
extern void (*alloc_init)(MemInfo*, uint32_t, uint32_t, STRING);
extern STRING (*alloc_mem)(MemInfo info, uint16_t id, uint32_t memsize);
extern int (*free_mem)(MemInfo info, uint16_t id, STRING);
extern void (*free_idmem)(MemInfo info, uint16_t id);
extern int (*change_id_mem)(MemInfo info, uint16_t oldid, uint16_t newid, STRING);
extern void (*ring_init)(RingInfo*, uint32_t, STRING);
extern int (*write_ring)(RingInfo*, STRING, uint32_t);
extern int (*read_ring)(RingInfo*, STRING, uint32_t);
extern void (*init_device)();
extern int32_t (*open_device)(uint16_t, STRING, uint32_t);
extern int32_t (*close_device)(uint16_t, uint32_t);
extern int32_t (*write_device)(uint32_t, STRING, uint32_t);
extern int32_t (*read_device)(uint32_t, STRING, uint32_t);
extern int32_t (*seek_device)(uint32_t, uint32_t);
extern int32_t (*ioctl_device)(uint32_t, uint32_t, uint32_t);
extern int32_t (*get_fd)(STRING);
extern int32_t (*stack_size)(STRING);
extern int32_t (*get_program_size)(STRING);
extern int32_t (*coff2bin)(STRING, STRING, void*, void*);
extern void (*set_handle)(int32_t (), void*, void*);
extern void (*init_que)(QueInfo*, uint16_t, uint16_t, void*);
extern int32_t (*que_size)(QueInfo*);
extern int32_t (*push_que)(QueInfo*, void*);
extern int32_t (*add_que)(QueInfo*, void*);
extern int32_t (*get_que)(QueInfo*, uint16_t, void*, int32_t (*)(void*));
extern int32_t (*set_que)(QueInfo*, uint16_t, void*);
extern int32_t (*del_que)(QueInfo*, uint16_t);
extern int32_t (*search_que)(QueInfo*, void*, int32_t (*)(void*, void*));
extern int32_t (*push_fifo)(QueInfo*, void*);
extern int32_t (*pop_fifo)(QueInfo*, void*);
extern STRING (*getptr_fifo)(QueInfo*, int32_t);
extern int32_t (*int_disable)(void);
extern int32_t (*int_enable)(void);
extern void (*get_meminfo)(uint32_t*, uint32_t*);
