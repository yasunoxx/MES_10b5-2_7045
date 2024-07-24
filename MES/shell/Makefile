SHCFLAGS = -O -m2 -r -fno-common

shell : shell.c Makefile
	sh-coff-gcc -O -m2 -T shshell.x -nostartfiles ../shcrt0.S shell.c ../check.c
	rm a.out
	sh-coff-gcc $(SHCFLAGS) -T shshell.x -nostartfiles ../shcrt0.S shell.c -o shell

clean :
	rm -f shell
