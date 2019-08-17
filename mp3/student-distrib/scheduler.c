#include "lib.h"
#include "paging.h"
#include "system.h"
#include "i8259.h"
#include "scheduler.h"
#include "pcb.h"
#include "x86_desc.h"
#include "paging.h"

/* Used to keep track of the process currently being scheduled */
static int32_t current_group;

/* scheduler_init
 *   DESCRIPTION: Initializes scheduler
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Scheduler begins with first terminal
 */
void scheduler_init()
{
    current_group = 0;
}

/* schedule_next 
 *   DESCRIPTION: Switches between processes using round-robin approach, across terminals 1-3. 
 *        INPUTS: proc_push_top _ indicates the top of the process' stack
                  pushed_cs - code segment register, gives privilege level
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Changes task state segment, pauses and unpauses processes 
 */
void schedule_next(uint32_t proc_push_top, uint32_t pushed_cs)
{
    cli();

    /* Accept another interrupt */
    send_eoi(IRQ_0);

    int32_t old_group = current_group;
    current_group++;

    /* current_group cycles from 0-2, wraps around */
    if (current_group >= NUM_OF_PROCESS_GROUPS)
    {
        current_group = 0;
    }

    /* Get PCB of process being paused */
    pcb_t* pcb_old = get_pcb_addr(active_pid[old_group]);
    /* Get PCB of process being unpaused */
    pcb_t* pcb_new = get_pcb_addr(active_pid[current_group]);

    /* Save data of old process: esp, ebp, esp0 */
    if ((pushed_cs & CPL_MASK) == CPL_3) {
        pcb_old->tss_esp0 = proc_push_top + (5 * ENTRY_SIZE);    /* 5 entries pushed */
    } else {
        pcb_old->tss_esp0 = proc_push_top + (3 * ENTRY_SIZE);    /* 3 entries pushed */
    }
    asm volatile(
        "movl %%esp, (%0)       \n\
         movl %%ebp, (%1)       \n"
        :
        : "r" (&(pcb_old->kernel_esp)),
          "r" (&(pcb_old->kernel_ebp))
        : "cc", "memory", "eax"
    );

    /* Remap user video page */
    if (pcb_new->vid_map_called)
    {
        if (visible_group == current_group)
        {
            map_page(VIDEO_USER, VIDEO_KERNEL, TRUE, TRUE, FALSE);
        }
        else
        {
            map_page(VIDEO_USER, VIDEO_GROUP_1 + current_group * PAGE_SIZE, TRUE, TRUE, FALSE);
        }
    }
    else
    {
        unmap_page(VIDEO_USER, FALSE);
    }
    
    /* Map <insert expletive> Process */
    map_page(PROG_VIRT_ADDR, get_prog_phys_addr(pcb_new->pid), TRUE, TRUE, TRUE);

    /* Restore task state segment */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = pcb_new->tss_esp0;

    /* Restore data of new proccess: ebp, esp */
    asm volatile(
        "movl (%0), %%esp               \n\
         movl (%1), %%ebp               \n"
        :
        : "r" (&(pcb_new->kernel_esp)),
          "r" (&(pcb_new->kernel_ebp))
        : "cc", "memory"
    );

    sti();
}

/* get_current_group 
 *   DESCRIPTION: Getter for current_group
 *        INPUTS: none
 *       OUTPUTS: current_group
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
int32_t get_current_group()
{
    return current_group;
}

/* set_current_group 
 *   DESCRIPTION: Setter for current_group
 *        INPUTS: pid -- new current_group value
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Changes current_group
 */
void set_current_group(int32_t pid)
{
    current_group = pid;
}
