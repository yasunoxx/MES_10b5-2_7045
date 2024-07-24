OUTPUT_FORMAT("coff-sh")
OUTPUT_ARCH(sh)
ENTRY("_start")
MEMORY
{
	ram(rwx)  : o = 0x00000, l = 0x10000
	stack(rw) : o = 0x00800, l = 0x00000
}

SECTIONS
{
.text : {
	*(.text)
	*(.strings)
	*(.rodata) 				
   	 _etext = . ; 
	} > ram
.tors : {
	___ctors = . ;
	*(.ctors)
	___ctors_end = . ;
	___dtors = . ;
	*(.dtors)
	___dtors_end = . ;
	}  > ram
.data : {
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
	*(.stack)
	}  > stack
}
