/* i8259.c - Functions to interact with the 8259 interrupt controller
* vim:ts=4 noexpandtab
*/

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* i8259_init 
 *  DESCRIPTION: Initialize the 8259 PIC
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Initializes the Master and Slave PICs
 */
void i8259_init(void) {
	long flags;
	cli_and_save(flags);

	master_mask = 0xFF;
	slave_mask = 0xFF;

	/* Mask all interrupts prior to init*/
	outb(0xFF, MASTER_8259_PORT_2);
	outb(0xFF, SLAVE_8259_PORT_2);

	/* Initalize Master */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_8259_PORT_2);
	outb(ICW3_MASTER, MASTER_8259_PORT_2);
	outb(ICW4, MASTER_8259_PORT_2);

	/* Initalize Slave */
	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_PORT_2);
	outb(ICW3_SLAVE, SLAVE_8259_PORT_2);
	outb(ICW4, SLAVE_8259_PORT_2);

	/* restore all interrupts prior to init */
	outb(master_mask, MASTER_8259_PORT_2);
	outb(slave_mask, SLAVE_8259_PORT_2);

	sti();
    restore_flags(flags);
}

/* enable_irq 
 *  DESCRIPTION: Enable (unmask) the specified IRQ
 *       INPUTS: irq_num IRQ Num from 0 - 15, important error checking is not done here
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Enables the IRQ line
 */
void enable_irq(uint32_t irq_num) {	
	long flags;
	cli_and_save(flags);
	if (irq_num & LINES_ON_PIC) { /* irq_num >= 8 */	
		/* IRQ is on slave */
		/* Clear bit for irq_num in slave mask */
		slave_mask &= ~(0x01 << (irq_num - LINES_ON_PIC));
		master_mask &= ~(0x01 << SLAVE_IRQ_LINE);
		outb(slave_mask, SLAVE_8259_PORT_2);
		outb(master_mask, MASTER_8259_PORT_2);
	} else {
		/* If IRQ on master */
		/* Clear bit for irq_num */
		master_mask &= ~(0x01 << irq_num);
		outb(master_mask, MASTER_8259_PORT_2);
	}
	sti();
    restore_flags(flags);
}

/* disable_irq 
 *  DESCRIPTION: Disable (mask) the specified IRQ 
 *       INPUTS: irq_num IRQ Num from 0 - 15, important error checking is not done here
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Disables the IRQ line
 */
void disable_irq(uint32_t irq_num) {
	long flags;
	cli_and_save(flags);
	if (irq_num & LINES_ON_PIC) {	/* irq_num >= 8 */	
		/* IRQ is on slave */
		/* Set bit for irq_num in slave mask */
		slave_mask |= 0x01 << (irq_num - LINES_ON_PIC);
		outb(slave_mask, SLAVE_8259_PORT_2);
	} else {
		/* If IRQ on master */
		/* Set bit for irq_num */
		master_mask |= 0x01 << irq_num;
		outb(master_mask, MASTER_8259_PORT_2);
	}
	sti();
    restore_flags(flags);
}

/* send_eoi 
 *  DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *       INPUTS: irq_num IRQ Num from 0 - 15, important error checking is not done here
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Sends EOI the IRQ line
 */
void send_eoi(uint32_t irq_num) {
	long flags;
	cli_and_save(flags);
	if(irq_num & LINES_ON_PIC) { /* irq_num >= 8 */	
		/* EOI on slave */
		/*&7 is to look at lower 3 bits*/
		outb(EOI + (irq_num&7), SLAVE_8259_PORT);
		outb(EOI + SLAVE_IRQ_LINE, MASTER_8259_PORT);
	} else {
		/* EOI on master */
		outb(EOI + irq_num, MASTER_8259_PORT);
	}
	sti();
    restore_flags(flags);
}
