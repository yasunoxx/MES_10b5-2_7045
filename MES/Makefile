SHCFLAGS = -O -m2
SRCS = main.c task.c request.c ip.c arp.c icmp.c udp.c tcp.c shm.c minifs.c fileio.c

all : com7045.mot

7045 : com7045.mot

com7045.mot : s_7045.mot m_7045.mot
	cat s_7045.mot m_7045.mot > com7045.mot
	rm s_7045.mot m_7045.mot

s_7045.mot : core_com.c print.c Makefile
	sh-coff-gcc $(SHCFLAGS) -DSH -DSH_7045 -T s_7045.x -nostartfiles ../shcrt0.S core_com.c print.c
	sh-coff-objcopy -O srec a.out s_7045.mot
	rm a.out

m_7045.mot : $(SRCS) Makefile map.h libsys_sh.a
	sh-coff-gcc $(SHCFLAGS) -DSH -T m_7045.x -nostartfiles ../shcrt0.S $(SRCS) -L. -lsys_sh
	sh-coff-objcopy -O srec a.out m_7045.mot
	rm a.out

libsys_sh.a :
	(cd libsys;make libsys_sh.a)

map.h : libsys_sh.a
	sh-coff-objdump -t libsys_sh.a | awk 'substr($$1,0,1) == "[" && $$2 == "1)(fl" && $$5 == "2)" {printf("extern %s();\n",substr($$9,2))} substr($$1,0,1) == "[" && $$3 == "1)(fl" && $$6 == "2)" {printf("extern %s();\n",substr($$10,2))}' > map.h
	sh-coff-objdump -t libsys_sh.a | awk 'BEGIN {print "static void* symptr[]={"} substr($$1,0,1) == "[" && $$2 == "1)(fl" && $$5 == "2)" {printf("\t%s,\n",substr($$9,2))} substr($$1,0,1) == "[" && $$3 == "1)(fl" && $$6 == "2)" {printf("\t%s,\n",substr($$10,2))} END {print "\t0\n};"}' >> map.h
	sh-coff-objdump -t libsys_sh.a | awk 'BEGIN {print "static char* symbol[]={"} substr($$1,0,1) == "[" && $$2 == "1)(fl" && $$5 == "2)" {printf("\t\"%s\",\n",$$9)} substr($$1,0,1) == "[" && $$3 == "1)(fl" && $$6 == "2)" {printf("\t\"%s\",\n",$$10)} END {print "\t0\n};"}' >> map.h
	sh-coff-objdump -t libsys_sh.a | awk 'substr($$1,0,1) == "[" && $$2 == "1)(fl" && $$5 == "2)" {printf("int %s(){}\n",substr($$9,2))} substr($$1,0,1) == "[" && $$3 == "1)(fl" && $$6 == "2)" {printf("int %s(){}\n",substr($$10,2))}' > check.c

clean :
	rm -f com7045.mot libsys*.a map.h check.c
	(cd libsys;make clean)
