SHCFLAGS = -O -m2 -mint32 -r -fno-common

http : http.c Makefile
	sh-coff-gcc -O -mh -mint32 -T ../shuser.x -nostartfiles ../shcrt0.S http.c ../check.c
	rm a.out
	sh-coff-gcc $(SHCFLAGS) -T ../shuser.x -nostartfiles ../shcrt0.S http.c -o http

clean : 
	rm -f http
