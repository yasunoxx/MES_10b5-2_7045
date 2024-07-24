/****************************************/
/* CoreOS/Copyleft Yukio Mituiwa,2003	*/
/*					*/
/*  2003/11/11 first release		*/
/*					*/
/****************************************/
#include "macro.h"
#include "pubtype.h"
#include "function.h"
#include "regdef.h"

#define IRQNUM 7
static void (*irq_proc[IRQNUM])();
static int32_t	params[IRQNUM];

#pragma interrupt
void int_irq0() {
	(*irq_proc[0])(params[0]);
}
#pragma interrupt
void int_irq1() {
	(*irq_proc[1])(params[1]);
}
#pragma interrupt
void int_irq2() {
	(*irq_proc[2])(params[2]);
}
#pragma interrupt
void int_irq3() {
	(*irq_proc[3])(params[3]);
}
#pragma interrupt
void int_irq4() {
	(*irq_proc[4])(params[4]);
}
#pragma interrupt
void int_irq5() {
	(*irq_proc[5])(params[5]);
}
#pragma interrupt
void int_irq6() {
	(*irq_proc[6])(params[6]);
}
#pragma interrupt
void int_irq7() {
	(*irq_proc[7])(params[7]);
}

int32_t alloc_irq(int32_t num, void (*proc)(int32_t), int32_t param) {
	int32_t	mask;

	if(num >= IRQNUM) return -1;
	mask = 0x80 >> num;
	if(ICR & mask) return -1;
	ICR |= mask;
	irq_proc[num] = proc;
	return 0;
}

int32_t free_irq(int32_t num) {
	int32_t	mask;

	if(num >= IRQNUM) return -1;
	mask = 0x80 >> num;
	if((ICR & mask) == 0) return -1;
	ICR &= ~mask;
	irq_proc[num] = 0;
	return 0;
}
