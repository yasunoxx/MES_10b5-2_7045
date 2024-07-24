SHCFLAGS = -O -m2
SRCS = alloc.c common.c exec.c stdio.c print.c net.c dhcp.c tftp.c shm.c misc.c version.c
OBJS = alloc.o common.o exec.o stdio.o print.o net.o dhcp.o tftp.o shm.o misc.o version.o

libsys_sh.a : $(SRCS)
	sh-coff-gcc $(SHCFLAGS) -DSH -c $(SRCS)
	sh-coff-ar r ../libsys_sh.a $(OBJS)
	sh-coff-ranlib ../libsys_sh.a
	rm -f *.o

clean :
	rm -f ../libsys*.a
