	.section .text
	.align 4
	.global _int_wovi
	.global _int_trap0
	.global _int_trap1
	.global _int_trap2
_int_wovi:
_int_trap0:
	mov.l	r0,@-r15
	mov.l	r1,@-r15
	mov.l	r2,@-r15
	mov.l	r3,@-r15
	mov.l	r4,@-r15
	mov.l	r5,@-r15
	mov.l	r6,@-r15
	mov.l	r7,@-r15
	mov.l	r8,@-r15
	mov.l	r9,@-r15
	mov.l	r10,@-r15
	mov.l	r11,@-r15
	mov.l	r12,@-r15
	mov.l	r13,@-r15
	mov.l	r14,@-r15
	sts.l	mach,@-r15
	sts.l	macl,@-r15
	sts.l	pr,@-r15
	mov.l	_stackpp_k,r0
	mov.l	@r0,r0
	mov.l	r15,@r0
	mov.l	_handlefunc_k,r0
	mov.l	@r0,r0
	jsr	@r0
	nop
_int_trap2:
	mov.l	_stackpp_k,r0
	mov.l	@r0,r0
	mov.l	@r0,r15
	lds.l	@r15+,pr
	lds.l	@r15+,macl
	lds.l	@r15+,mach
	mov.l	@r15+,r14
	mov.l	@r15+,r13
	mov.l	@r15+,r12
	mov.l	@r15+,r11
	mov.l	@r15+,r10
	mov.l	@r15+,r9
	mov.l	@r15+,r8
	mov.l	@r15+,r7
	mov.l	@r15+,r6
	mov.l	@r15+,r5
	mov.l	@r15+,r4
	mov.l	@r15+,r3
	mov.l	@r15+,r2
	mov.l	@r15+,r1
	mov.l	@r15+,r0
	rte
	nop
_int_trap1:
	mov.l	_curtaskp_k,r0
	mov.l	@r0,r0
	mov.l	@r0,r0
	rte
	nop

	.align 4
_stackpp_k:
	.long	_stackpp
_handlefunc_k:
	.long	_handlefunc
_curtaskp_k:
	.long	_curtaskp
	.end
