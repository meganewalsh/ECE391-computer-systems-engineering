#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "types.h"

#define NUM_OF_PROCESS_GROUPS   3
#define IRQ_0                   0
#define CPL_3                   0x03
#define CPL_MASK                0x03
#define ENTRY_SIZE              4

int visible_group;                                  /* Currently Visible Terminal - Starts at 0 */

void schedule_next(uint32_t proc_push_top, uint32_t pushed_cs);
int32_t get_current_group();
void set_current_group(int32_t pid);
void scheduler_init();


#endif
