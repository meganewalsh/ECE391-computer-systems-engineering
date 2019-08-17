#include "pit.h"
#include "lib.h"
#include "i8259.h"
#include "scheduler.h"

/* pit_init
 *   DESCRIPTION: Initializes the PIT by writing the reload value to the port for
                  Channel 0 and selecting a channel, access mode, operating mode,
                  and bcd/binary mode.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: PIT produces interrupts on IRQ 0 every 25 ms
 */
void pit_init()
{
    cli();

    /* Channel 0 data port used to set the counter's 16 bit reload value (10-50ms)
       or read the channel's 16 bit current count */ 
    outb(RELOAD_VAL, CH_0_DATA_PORT);
 
    /* Mode/Command port requires channel selection (00), access mode (01, lobyte bc max 8 bits),
       operating mode (011, square wave generator), and bcd/binary mode (0, binary) */
    outb(MODE_COMMAND, MODE_COMMAND_PORT);

    sti();

    /* Allow interrupts */
    enable_irq(IRQ_0);
}

