/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
int int_enable() {
	int	ret, reg;

	asm("stc sr,%0":"=r"(ret):);
	reg = ret & 0xffffff0f;
	asm("ldc %0,sr"::"r"(reg));
	return ret;
}

int int_disable() {
	int	ret, reg;

	asm("stc sr,%0":"=r"(ret):);
	reg = ret | 0xf0;
	asm("ldc %0,sr"::"r"(reg));
	return ret;
}

void set_flag(int flag) {
	asm("ldc %0,sr"::"r"(flag));
}
