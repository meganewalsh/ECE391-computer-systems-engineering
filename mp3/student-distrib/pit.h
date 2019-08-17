#ifndef PIT_H_
#define PIT_H_

#include "types.h"

#define IRQ_0               0           /* IRQ number for PIT */
#define CH_0_DATA_PORT      0x40        /* Channel 0 port */
#define MODE_COMMAND_PORT   0x43        /* Mode/Command port */
#define RELOAD_VAL          40          /* currently 25ms -- val must be between 20 (50ms) and 100 (10ms) */
#define MODE_COMMAND        00010110    /* 8-bit value specifying chosen modes */

void pit_init();

#endif
