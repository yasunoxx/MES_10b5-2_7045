	.section .text
	.global	_start
	.global	___main
_start:
	mov.l	stack_k,r15
! move ram data
	mov.l	dtors_end_k,r0
	mov.l	data_k,r1
	mov.l	edata_k,r2
move_data:
	mov.l	@r0,r3
	mov.l	r3,@r1
	add	#4,r0
	add	#4,r1
	cmp/ge	r1,r2
	bt	move_data
	nop
! call the mainline	
	mov.l	main_k,r0
	jsr	@r0
	or	r0,r0
! call exit
	mov.l	start_k,r0
	jmp	@r0
___main:
	rts
	.align 2
start_k:
	.long	_start	
stack_k:
	.long	_stack	
led_k:
	.long	0xc00001
dtors_end_k:
	.long	___dtors_end	
data_k:
	.long	___data
edata_k:
	.long	_edata
main_k:
	.long	_main
