OUTPUT_FORMAT("coff-sh")
OUTPUT_ARCH(sh)
ENTRY("_start")
MEMORY
{
	syscall(r) : o = 0x00000500, l = 0x00200
	rom(rx)    : o = 0x00014000, l = 0x0c000
	ram(rwx)   : o = 0xffff6d00, l = 0x02000
	stack(rw)  : o = 0xffffdffc, l = 0x00004
}

SECTIONS
{
.syscall : {
	_alloc_init = . + 0;
	_alloc_mem = . + 4;
	_free_mem = . + 8;
	_free_idmem = . + 12;
	_change_id_mem = . + 16;
	_ring_init = . + 20;
	_write_ring = . + 24;
	_read_ring = . + 28;
	_init_device = . + 32;
	_open_device = . + 36;
	_close_device = . + 40;
	_write_device = . + 44;
	_read_device = . + 48;
	_seek_device = . + 52;
	_ioctl_device = . + 56;
	_get_fd = . + 60;
	_stack_size = . + 64;
	_get_program_size = . + 68;
	_coff2bin = . + 72;
	_set_handle = . + 76;
	_init_que = . + 80;
	_que_size = . + 84;
	_push_que = . + 88;
	_add_que = . + 92;
	_get_que = . + 96;
	_set_que = . + 100;
	_del_que = . + 104;
	_search_que = . + 108;
	_push_fifo = . + 112;
	_pop_fifo = . + 116;
	_getptr_fifo = . + 120;
	_int_disable = . + 124;
	_int_enable = . + 128;
	_get_meminfo = . + 132;
	FILL(0xff)
        }  > syscall
.text : {
	*(.text)
	*(.strings)
	*(.rodata) 				
   	 _etext = . ; 
	} > rom
.tors : {
	___ctors = . ;
	*(.ctors)
	___ctors_end = . ;
	___dtors = . ;
	*(.dtors)
	___dtors_end = . ;
	}  > rom
.data : AT ( ADDR(.tors) + SIZEOF(.tors) ){
	___data = . ;
	*(.data)
	 _edata = .;
	} > ram
.bss : {
	 _bss_start = . ;
	*(.bss)
	 _end = . ;  
	}  >ram
.stack : {
	 _stack = . ; 
	}  > stack
}
