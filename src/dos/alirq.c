/* derived from allegro 4.0.3 By Shawn Hargreaves */
#include <stdio.h>
#include <stdlib.h>
#include <dpmi.h>
#include <go32.h>


#define MAX_IRQS     2           /* timer + soundcard */
#define STACK_SIZE   8192        /* an 8k stack should be plenty */


struct irq_handler {
	int (*handler)(void);		/* our C handler */
	int number;					/* irq number */
	__dpmi_paddr old_vector;	/* original protected mode vector */
};


struct irq_handler allegro_irq_handler[MAX_IRQS];

unsigned char *allegro_irq_stack[MAX_IRQS];

void allegro_irq_wrapper_0(void);
void allegro_irq_wrapper_1(void);


void allegro_irq_init(void)
{
	int i;

	for(i=0; i<MAX_IRQS; i++) {
		allegro_irq_handler[i].handler = 0;
		allegro_irq_handler[i].number = 0;
	}

	for(i=0; i<MAX_IRQS; i++) {
		if((allegro_irq_stack[i] = malloc(STACK_SIZE))) {
			allegro_irq_stack[i] += STACK_SIZE - 32;   /* stacks grow downwards */
		}
	}
}


void allegro_irq_exit(void)
{
	int i;

	for(i=0; i<MAX_IRQS; i++) {
		if(allegro_irq_stack[i]) {
			allegro_irq_stack[i] -= STACK_SIZE - 32;
			free(allegro_irq_stack[i]);
			allegro_irq_stack[i] = 0;
		}
	}
}


/* _install_irq:
 *  Installs a hardware interrupt handler for the specified irq, allocating
 *  an asm wrapper function which will save registers and handle the stack
 *  switching. The C function should return zero to exit the interrupt with
 *  an iret instruction, and non-zero to chain to the old handler.
 */
int _install_irq(int num, int (*handler)(void))
{
	__dpmi_paddr addr;
	int i;

	for(i=0; i<MAX_IRQS; i++) {
		if(!allegro_irq_handler[i].handler) {
			addr.selector = _my_cs();

			switch(i) {
			case 0:
				addr.offset32 = (long)allegro_irq_wrapper_0;
				break;
			case 1:
				addr.offset32 = (long)allegro_irq_wrapper_1;
				break;
			default:
				return -1;
			}

			allegro_irq_handler[i].handler = handler;
			allegro_irq_handler[i].number = num;

			__dpmi_get_protected_mode_interrupt_vector(num, &allegro_irq_handler[i].old_vector);
			__dpmi_set_protected_mode_interrupt_vector(num, &addr);
			return 0;
		}
	}

	return -1;
}



/* _remove_irq:
 *  Removes a hardware interrupt handler, restoring the old vector.
 */
void _remove_irq(int num)
{
	int i;

	for(i=0; i<MAX_IRQS; i++) {
		if(allegro_irq_handler[i].number == num) {
			__dpmi_set_protected_mode_interrupt_vector(num, &allegro_irq_handler[i].old_vector);
			allegro_irq_handler[i].number = 0;
			allegro_irq_handler[i].handler = 0;
			break;
		}
	}
}
