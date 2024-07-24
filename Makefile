SRCS = main.c que.c sci.c device.c ring.c alloc.c coff.c set_handle.c config.c irq.c bus.c ne.c pcmcia.c romdisk.c lcd.c ramdisk.c i2c.c seeprom.c sl811.c

SHCFLAGS = -O -m2
SRCSSH = sh.c handle_sh.s

SRCS7045 = $(SRCS) $(SRCSSH) rom_gen.c
all : sys7045.mot

sys7045.mot : core7045.mot conf7045.mot
	(cd MES;make 7045)
	cat core7045.mot MES/com7045.mot conf7045.mot > sys7045.mot
	rm core7045.mot conf7045.mot

core7045.mot : $(SRCS7045) Makefile
	sh-coff-gcc $(SHCFLAGS) -DSH -DSH_7045 -T core7045.x -nostartfiles shcrt0.S $(SRCS7045) -lc
	sh-coff-objcopy -O srec a.out core7045.mot
	rm a.out

conf7045.mot : conf7045.def
	tool/config < conf7045.def > conf7045.mot

clean :
	rm -f sys*.mot
	(cd MES;make clean)
