/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
int	(*handlefunc)();
void	*stackpp, *curtaskp;

void set_handle(int (*func)(), void *spp, void *ctp) {
	handlefunc = func;
	stackpp = spp;
	curtaskp = ctp;
}
