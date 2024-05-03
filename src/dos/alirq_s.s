# derived from allegro 4.0.3 By Shawn Hargreaves
	.text

	.equ IRQ_STACKS, 2
# 32bit cbaddr + 32bit number + 6-byte seg/offs old_vector
	.equ IRQ_SIZE, 16
# irq_handler struct offsets
	.equ IRQ_HANDLER, 0
	.equ IRQ_NUMBER, 4
	.equ IRQ_OLDVEC, 8

	.extern ___djgpp_ds_alias
	.extern _allegro_irq_stack
	.extern _allegro_irq_handler

	.macro WRAPPER x
	.globl _allegro_irq_wrapper_\x
_allegro_irq_wrapper_\x:
	pushw %ds	# save registers
	pushw %es
	pushw %fs
	pushw %gs
	pushal

	.byte 0x2e	# cs: override
	movw ___djgpp_ds_alias, %ax
	movw %ax, %ds	# set up selectors
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
									
	movl $IRQ_STACKS-1, %ecx               # look for a free stack
									
stack_search_loop_\x:
	leal _allegro_irq_stack(, %ecx, 4), %ebx
	cmpl $0, (%ebx)
	jnz found_stack_\x                    # found one!
									
	decl %ecx
	jge stack_search_loop_\x
									
	jmp get_out_\x                        # oh shit..
									
found_stack_\x:
	movl %esp, %ecx                        # old stack in ecx + dx
	movw %ss, %dx
									
	movl (%ebx), %esp                      # set up our stack
	movw %ax, %ss
									
	movl $0, (%ebx)                        # flag the stack is in use
									
	pushl %edx                             # push old stack onto new
	pushl %ecx
	pushl %ebx
									
	cld                                    # clear the direction flag
									
	movl _allegro_irq_handler + IRQ_HANDLER + IRQ_SIZE * \x, %eax
	call *%eax                             # call the C handler
									
	cli
									
	popl %ebx                              # restore the old stack
	popl %ecx
	popl %edx
	movl %esp, (%ebx)
	movw %dx, %ss
	movl %ecx, %esp
									
	orl %eax, %eax                         # check return value
	jz get_out_\x
									
	popal                                  # chain to old handler
	popw %gs
	popw %fs
	popw %es
	popw %ds
	ljmp *%cs:_allegro_irq_handler + IRQ_OLDVEC + IRQ_SIZE * \x
									
get_out_\x:
	popal                                  # iret
	popw %gs
	popw %fs
	popw %es
	popw %ds
	sti
	iret
	.endm


WRAPPER 0
WRAPPER 1
