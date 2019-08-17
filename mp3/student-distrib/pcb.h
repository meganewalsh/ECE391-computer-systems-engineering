#ifndef PCB_H_
#define PCB_H_

#include "types.h"
#include "file.h"
// #include "term.h"

#define MAX_PROCESS_GROUPS  3               /* Number of process groups */
#define MAX_PID             7               /* 6 processes plus the kernel (PID 0) */
#define PCB_BLK_SIZE        0x2000          /* 8 KiB */
#define TERM_BUFFER_SIZE    128

int32_t active_pid[MAX_PROCESS_GROUPS];  /* PIDs of leaf processes of each process group */

typedef struct process_control_block {
    int32_t pid;
    int32_t parent_pid;
    uint32_t esp;                       /* ESP of this process */
    uint32_t ebp;                       /* EBP of this process */
    uint32_t eip;                       /* Entry point or return address when a child is spawned */
    uint32_t kernel_esp;                /* Kernel's ESP while process is waiting to be scheduled */
    uint32_t kernel_ebp;                /* Kernel's EBP while process is in waiting to be scheduled */
    uint32_t tss_esp0;                  /* holds tss esp0 */
    file_t fd_table[FD_ARRAY_SIZE];
    uint8_t args[TERM_BUFFER_SIZE];     /* Program arguments */
    uint8_t args_len;
    uint8_t vid_map_called;             /* 0 if user vidmem page is not mapped, 1 if is mapped */
} pcb_t;

extern void pcb_init();
extern pcb_t* pcb_setup();
extern void pcb_teardown();
extern pcb_t* get_current_pcb();
extern int get_new_fd();
extern pcb_t* get_pcb_addr(int32_t pid);
extern uint32_t get_kstack_addr(int32_t pid);
extern int32_t get_new_pid();

#endif /* PCB_H_ */
