SRCS = main.c que.c sci.c device.c ring.c alloc.c coff.c set_handle.c config.c irq.c bus.c ne.c pcmcia.c romdisk.c lcd.c ramdisk.c i2c.c seeprom.c sl811.c

SHCFLAGS = -O -m2
SRCSSH = sh.c handle_sh.s

SRCS7045 = $(SRCS) $(SRCSSH) rom_gen.c
all : sys7045.mot

sys7045.mot : core7045.mot conf7045.mot
	(cd MES;make 7045)
	echo "S00F0000636F7265373034352E6D6F74F9" > sys7045.mot
	cat core7045.mot | awk /S3/'{print $0}' > sys7045.tmp
	cat MES/com7045.mot | awk /S3/'{print $0}' >> sys7045.tmp
	cat conf7045.mot | awk /S3/'{print $0}' >> sys7045.tmp
	sort sys7045.tmp >> sys7045.mot
	echo "S70500000700F3" >> sys7045.mot

core7045.mot : $(SRCS7045) Makefile
	sh-coff-gcc $(SHCFLAGS) -DSH -DSH_7045 -T core7045.x -nostartfiles shcrt0.S $(SRCS7045) -lc
	sh-coff-objcopy -O srec --srec-forceS3 a.out core7045.mot
	rm a.out

conf7045.mot : conf7045.def
	(cd tool;make)
	tool/config < conf7045.def > conf7045.tmp
	sh-coff-objcopy -I srec -O srec --srec-forceS3 conf7045.tmp conf7045.mot

clean :
	rm -f *.mot *.tmp
	(cd tool;make clean)
	(cd MES;make clean)
