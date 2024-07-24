	.section .text
	.align	4
	.global	___main
	.global	_start
_start:
	sts.l	pr,@-r15
	mov.l	r0,@-r15
	mov.l	main_k,r0
	jsr	@r0
	nop
	mov.l	exit_k,r0
	jsr	@r0
	nop
	mov.l	@r15+,r0
	lds.l	@r15+,pr
	rts
	nop
___main:
	rts
	nop
	.align 4
main_k:
	.long	_main
exit_k:
	.long	_s_exit
led_k:
	.long	0xc00001
	.end
