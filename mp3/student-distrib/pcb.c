#include "lib.h"
#include "pcb.h"
#include "scheduler.h"
#include "term.h"
#include "x86_desc.h"

#define KERNEL_LOC_END      0x800000


/* Local variables */
static pcb_t* pid_array[MAX_PID];   /* Pointers to MAX_PID PCBs of active processes */

/*
 * pcb_init
 *   DESCRIPTION: Initializes pid_array and does custom setup of kernel's PCB.
 *                This PCB allows the first program to be properly spawned.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Initializes pid_array, current_pid, and PID 0's PCB.
 */
void pcb_init()
{
    int i;

    /* Clear out pids */
    for (i = 0; i < MAX_PID; ++i)
    {
        pid_array[i] = NULL;
    }

    /* Set active PIDs to 0 */
    for (i = 0; i < NUM_OF_PROCESS_GROUPS; ++i)
    {
        active_pid[i] = 0;
    }

    /* Set up PCB for kernel so first child (shell) may be properly spawned */
    pcb_t* pcb = get_pcb_addr(active_pid[get_current_group()]);
    pid_array[active_pid[get_current_group()]] = pcb;

    /* Fill in PCB */
    pcb->pid = active_pid[get_current_group()];
    pcb->parent_pid = -1;       /* Kernel has no parent */

    /* Open stdin and stdout */
    term_open((const uint8_t*)"stdin");
    term_open((const uint8_t*)"stdout");
    
    /* Initialize rest of FD array */
    for (i = 2; i < FD_ARRAY_SIZE; ++i)
    {
        pcb->fd_table[i].flags = NOT_IN_USE;
    }

    /* Clear argument length field */
    pcb->args_len = 0;
}

/*
 * pcb_setup
 *   DESCRIPTION: Gets next available PID, updates pid_array to associated PCB
 *                location, updates the current PID, fills the new PCB with the
 *                child and parent PIDs, initializes the FD array, and opens
 *                stdin/stdout.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: Pointer to newly setup PCB or NULL if no new PIDs
 *  SIDE EFFECTS: Updates pid_array, current_group, and tss.esp0
 */
pcb_t* pcb_setup(int32_t child_pid)
{
    int i;
    int32_t parent_pid;
    int32_t current_group = get_current_group();
    pcb_t* child_pcb;
    
    if (child_pid == FAILURE)
        return NULL;

    child_pcb = get_pcb_addr(child_pid);            /* Get PCB location */
    pid_array[child_pid] = child_pcb;               /* Set PCB location in PID array */
    parent_pid = active_pid[current_group];         /* Save parent PID */
    active_pid[current_group] = child_pid;          /* Update current PID */

    /* Clear Memory in PCB prior to filling in */
    memset(child_pcb, 0, sizeof(pcb_t));

    /* Fill in PCB */
    child_pcb->pid = child_pid;
    child_pcb->parent_pid = parent_pid;

    /* Open stdin and stdout */
    term_open((const uint8_t*)"stdin");
    term_open((const uint8_t*)"stdout");
    
    /* Initialize rest of FD array */
    for (i = 2; i < FD_ARRAY_SIZE; ++i)
    {
        child_pcb->fd_table[i].flags = NOT_IN_USE;
    }

    /* Clear argument length field */
    child_pcb->args_len = 0;

    return child_pcb;
}

/*
 * pcb_teardown
 *   DESCRIPTION: Closes all open FDs including stdin/stdout, clears the
 *                associated pid_array entry, updates the current_pid to that
 *                of the parent process, updates esp0 in the TSS to point to
 *                the parent's kernel stack, and clears the PCB's memory.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Updates pid_array, current_pid, and tss.esp0
 */
void pcb_teardown()
{
    int i;
    pcb_t* pcb = get_current_pcb();
    
    /* Close all files that are still open */
    for (i = 0; i < FD_ARRAY_SIZE; ++i)
    {
        if (pcb->fd_table[i].flags == IN_USE && pcb->fd_table[i].file_ops->close != NULL)
        {
            pcb->fd_table[i].file_ops->close(i);
        }
    }

    /* Clear pid array entry and update current pid */
    pid_array[active_pid[get_current_group()]] = NULL;
    active_pid[get_current_group()] = pcb->parent_pid;

    /* Update esp0 to point to parent's kstack */
    tss.esp0 = get_kstack_addr(active_pid[get_current_group()]);    

    /* Clear PCB */
    memset(pcb, 0, sizeof(pcb_t));
}

/*
 * get_current_pcb
 *   DESCRIPTION: Returns pointer to the PCB of the current process.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: Pointer to the PCB of the current process
 *  SIDE EFFECTS: none
 */
pcb_t* get_current_pcb()
{
    return pid_array[active_pid[get_current_group()]];
}

/*
 * get_new_fd
 *   DESCRIPTION: Gets next available file descriptor index from PCB of current
 *                process. (Does not mark returned FD as in use.)
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: Next available FD or FAILURE if no available FDs
 *  SIDE EFFECTS: none
 */
int get_new_fd()
{
    int fd;
    for (fd = 0; fd < FD_ARRAY_SIZE; ++fd)
    {
        if (get_current_pcb()->fd_table[fd].flags == NOT_IN_USE)
            return fd;
    }

    return FAILURE;
}

/*
 * Get PCB location based on PID
 * get_pcb_addr
 *   DESCRIPTION: Gets pointer to PCB based on given PID.
 *        INPUTS: pid - process ID number
 *       OUTPUTS: none
 *  RETURN VALUE: Pointer to PID's PCB
 *  SIDE EFFECTS: none
 */
pcb_t* get_pcb_addr(int32_t pid)
{
    if (pid < 0 || pid >= MAX_PID)
        return NULL;

    return (pcb_t*)(KERNEL_LOC_END - PCB_BLK_SIZE * (pid + 1));
}

/*
 * get_kstack_addr
 *   DESCRIPTION: Gets address to base of PID's PCB block from which its kernel
 *                stack grows.
 *        INPUTS: pid - process ID number
 *       OUTPUTS: none
 *  RETURN VALUE: Address of kernel stack base
 *  SIDE EFFECTS: none
 */
uint32_t get_kstack_addr(int32_t pid)
{
    if (pid < 0 || pid >= MAX_PID)
        return NULL;

    /* Kernel stack is a word (4 bytes) from the bottom of the corresponding PCB block in the kernel page */
    return (uint32_t)(KERNEL_LOC_END - PCB_BLK_SIZE * pid - 4);
}

/*
 * get available pid
 * get_new_pid
 *   DESCRIPTION: Gets next available PID in the pid_array.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: Next available PID or FAILURE if no available PIDs
 *  SIDE EFFECTS: none
 */
int32_t get_new_pid()
{
    int32_t pid;
    for (pid = 0; pid < MAX_PID; ++pid)
    {
        if (pid_array[pid] == NULL)
            return pid;
    }
    
    return FAILURE;
}
