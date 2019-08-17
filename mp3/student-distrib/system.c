#include "file_sys.h"
#include "idt.h"
#include "lib.h"
#include "paging.h"
#include "pcb.h"
#include "rtc.h"
#include "system.h"
#include "scheduler.h"
#include "term.h"
#include "x86_desc.h"

/* Local helper functions */
int32_t __load_program(const uint8_t* filename);
int32_t __validate_user_ptr(uint32_t ptr);

/* System call linkage. Immediately saves registers before calling dispatcher */
asm(
    ".global system_call_handler                    \n\
    system_call_handler:                            \n\
        pushl %edi                                  \n\
        pushl %esi                                  \n\
        pushl %ebp                                  \n\
        pushl %esp                                  \n\
        pushl %edx                                  \n\
        pushl %ecx                                  \n\
        pushl %ebx                                  \n\
        cld                                         \n\
        call do_system_call                         \n\
        jmp system_call_handler_return              \n"
);

/* System call dispatcher. Returns with -1 for invalid system call numbers */
asm(
    "do_system_call:                                \n\
        cmpl $8, %eax                               \n\
        ja  system_call_handler_failure             \n\
        cmpl $0, %eax                               \n\
        je  system_call_handler_failure             \n\
        jmp *system_call_jump_table(, %eax, 4)      \n\
                                                    \n\
    system_call_handler_failure:                    \n\
        movl $-1, %eax                              \n\
        leave                                       \n\
        ret                                         \n"
);

/* Restore registers and return from system call */
asm(
    "system_call_handler_return:                    \n\
        popl %ebx                                   \n\
        popl %ecx                                   \n\
        popl %edx                                   \n\
        popl %esp                                   \n\
        popl %ebp                                   \n\
        popl %esi                                   \n\
        popl %edi                                   \n\
        jmp  iret_and_save_tss_esp                  \n"
);

/* Jump table to system call functions */
asm(
    "system_call_jump_table:                                \n\
        .long 0, system_halt, system_execute, system_read   \n\
        .long system_write, system_open, system_close       \n\
        .long system_getargs, system_vidmap                 \n"
);

/*
 * system_halt
 *   DESCRIPTION: Halts currently executing program by switching back to the
 *                parent's context.
 *        INPUTS: status - halt code to return to the parent process
 *       OUTPUTS: none
 *  RETURN VALUE: Passes halt code to parent through EAX
 *  SIDE EFFECTS: Switches context to parent
 */
int32_t system_halt(uint32_t status)
{
    /* Get parent's PCB */
    pcb_t* parent_pcb = get_pcb_addr(get_current_pcb()->parent_pid);
    
    /* Set up IRET context and IRET to return_from_exec */
    asm volatile(
        "movl %0, %%eax             \n\
        pushl %%ss                 \n\
        pushl 8(%%edx)             \n\
        pushf                      \n\
        pushl %%cs                 \n\
        pushl 16(%%edx)            \n\
        iret                       \n"
        :
        : "r" (status),
          "d" (parent_pcb)
        : "cc", "memory"
    );

    /* Should IRET and never reach this point */
    return FAILURE;
}

/*
 * system_execute
 *   DESCRIPTION: Executes program given in command by spawning a child
 *                process, loading the appropriate executable file into memory,
 *                and switching into that process' context. Execute does not
 *                return to the parent until the child has halted.
 *        INPUTS: command - string of executable name and three arguments
 *       OUTPUTS: none
 *  RETURN VALUE: Return/halt code of executed process.
 *  SIDE EFFECTS: Child process is spawned
 */
int32_t system_execute(const uint8_t* command)
{
    cli();

    /* Command must be a valid pointer */
    if (command == NULL)
        return FAILURE;

    /* Spin to find first non-space character */
    while (command[0] == ' ') {
        ++command;
    }

    /* Copy filename name into buffer */
    uint8_t filename[TERM_BUFFER_SIZE];
    int cmd_idx = 0;
    while (cmd_idx < TERM_BUFFER_SIZE-1 && (char)(command[cmd_idx]) != ' ' && (char)(command[cmd_idx]) != '\0')
    {
        filename[cmd_idx] = command[cmd_idx];
        ++cmd_idx;
    }
    filename[cmd_idx] = '\0';

    /* Create child PCB */
    int32_t child_pid = get_new_pid();
    pcb_t* parent_pcb = get_current_pcb();
    pcb_t* child_pcb = pcb_setup(child_pid);
    if (child_pcb == NULL)
        return FAILURE;

    /* Parse and save arguments in PCB */
    int args_idx = 0;
    int arg_num;

    /* Spin to find first arg */
    while (command[cmd_idx] == ' ')
        ++cmd_idx;

    /* For each of 3 possible arguments */
    for (arg_num = 0; arg_num < MAX_NUM_ARGS; ++arg_num)
    {
        /* Check for zero arguments */
        if (command[cmd_idx] == '\0')
        {
            child_pcb->args[0] = '\0';
            break;
        }
        
        /* Copy arg into PCB */
        while (cmd_idx < TERM_BUFFER_SIZE-1 && (char)(command[cmd_idx]) != ' ' && (char)(command[cmd_idx]) != '\0')
        {
            child_pcb->args[args_idx] = command[cmd_idx];
            ++args_idx;
            ++cmd_idx;
        }

        /* Spin to find next arg */
        while (command[cmd_idx] == ' ')
            ++cmd_idx;

        /* If end of command, add NULL terminator and stop parsing */
        if ((char)(command[cmd_idx]) == '\0')
        {
            child_pcb->args[args_idx] = '\0';
            ++args_idx;
            break;
        }
        /* Insert space between each arg */
        else
        {
            child_pcb->args[args_idx] = ' ';
            ++args_idx;
        }
    }

    /* Save length of argument buffer in PCB */
    child_pcb->args_len = args_idx;

    /* Map program page */
    map_page(PROG_VIRT_ADDR, get_prog_phys_addr(child_pcb->pid), TRUE, TRUE, TRUE);

    /* Load program */
    uint32_t program_eip = __load_program(filename);
    if (program_eip == FAILURE) 
    {
        /* Clean up */
        map_page(PROG_VIRT_ADDR, get_prog_phys_addr(parent_pcb->pid), TRUE, TRUE, TRUE);
        pcb_teardown();     /* Reverts current pid to parent */
        return FAILURE;
    }

    /* Update TSS's esp0 to point to child's kstack */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = get_kstack_addr(child_pid);

    /* Set EIP in PCB to entry point of program */
    child_pcb->eip = program_eip;

    /* Set ESP in PCB to a word (4 bytes) above the bottom of process page */
    child_pcb->esp = PROG_VIRT_ADDR + PROG_PAGE_SIZE - 4;

    /* IP, ESP, and EBP in parent PCB to facilitate parent process resume */
    asm volatile(
        "movl %%esp, (%0)               \n\
         movl %%ebp, (%1)               \n\
         movl $return_from_exec, (%2)   \n"
        :
        : "r" (&(parent_pcb->esp)),
          "r" (&(parent_pcb->ebp)),
          "r" (&(parent_pcb->eip))
        : "cc", "memory"
    );

    //sti();  // flags set in assembly below

    /* Context switch */
    /* push xss
     * push esp
     * push eflags
     * push xcs
     * push return address (eip, entry point into program)
     * iret
     * */
    asm volatile(
        "movl  %%eax, %%ds  \n\
         pushl %%eax        \n\
         pushl %%ebx        \n\
         pushf              \n\
         orl   $0x200, (%%esp) \n\
         pushl %%ecx        \n\
         pushl %%edx        \n\
         iret               \n"
         :
         : "a" (USER_DS),
           "b" (child_pcb->esp),
           "c" (USER_CS),
           "d" (child_pcb->eip)
         : "cc", "memory"
    );

    /* Return from execute */
    /* Restore parent's esp and ebp */
    int ret = 0;
    asm volatile(
        "return_from_exec:          \n\
            movl 8(%%edx), %%esp    \n\
            movl 12(%%edx), %%ebp   \n"
        : "=a" (ret)
        : "d" (parent_pcb)
        : "cc", "memory"
    );

    /* Page switch: remap process page to parent's physical page */
    map_page(PROG_VIRT_ADDR, get_prog_phys_addr(parent_pcb->pid), TRUE, TRUE, TRUE);

    /* Unmap vid_map page if parent has not called vidmap system call */
    if (parent_pcb->vid_map_called == 0)
    {
        unmap_page(VIDEO_USER, FALSE);
    }

    /* Clean up child's PCB */
    pcb_teardown();

    return ret;
}

/*
 * system_read
 *   DESCRIPTION: Calls appropriate read function from the given FD's
 *                file_op_table.
 *        INPUTS: fd - file descriptor index
 *                buf - parameter to file's read function
 *                nbytes - parameter to file's read function
 *       OUTPUTS: none
 *  RETURN VALUE: Return value of file's read function or FAILURE for invalid
 *                FD or no read function.
 *  SIDE EFFECTS: Calls file's read function.
 */
int32_t system_read(int32_t fd, void* buf, int32_t nbytes)
{
    file_t* fd_array = get_current_pcb()->fd_table;

    /* Check for valid fd */
    if (fd < 0 || fd >= FD_ARRAY_SIZE || fd_array[fd].flags == NOT_IN_USE)
        return FAILURE;

    /* Call appropriate read functiosn in fd's op table */
    if (fd_array[fd].file_ops->read != NULL)
        return fd_array[fd].file_ops->read(fd, buf, nbytes);
    
    return FAILURE;
}

/*
 * system_write
 *   DESCRIPTION: Calls appropriate write function from the given FD's
 *                file_op_table.
 *        INPUTS: fd - file descriptor index
 *                buf - parameter to file's write function
 *                nbytes - parameter to file's write function
 *       OUTPUTS: none
 *  RETURN VALUE: Return value of file's write function or FAILURE for invalid
 *                FD or no write function.
 *  SIDE EFFECTS: Calls file's write function.
 */
int32_t system_write(int32_t fd, const void* buf, int32_t nbytes)
{
    file_t* fd_array = get_current_pcb()->fd_table;

    /* Check for valid fd */
    if (fd < 0 || fd >= FD_ARRAY_SIZE || fd_array[fd].flags == NOT_IN_USE)
        return FAILURE;

    /* Call appropriate write function in fd's op table */
    if (fd_array[fd].file_ops->write != NULL)
        return fd_array[fd].file_ops->write(fd, buf, nbytes);
    
    return FAILURE;
}

/*
 * system_open
 *   DESCRIPTION: For a given, valid filename, the next available FD is
 *                populated with the file_op_table based on the dentry's
 *                filetype. Then file's open function is called.
 *        INPUTS: filename - file to open
 *       OUTPUTS: none
 *  RETURN VALUE: Assigned file descriptor index or FAILURE if invalid filename,
 *                no available FDs, or FD has no associated open function.
 *  SIDE EFFECTS: Fills entry in fd_table; calls files open function.
 */
int32_t system_open(const uint8_t* filename)
{
    dentry_t dentry;

    /* Check for valid filename */
    if (read_dentry_by_name(filename, &dentry) != SUCCESS)
        return FAILURE;

    /* Get next available file descriptor index from PCB */
    int fd = get_new_fd();
    if (fd == FAILURE)
        return FAILURE;

    /* Populate file descriptor index */
    file_t* fd_array = get_current_pcb()->fd_table;
    fd_array[fd].inode = dentry.inode_num;
    fd_array[fd].file_position = INIT_FILE_POS;
    fd_array[fd].flags = IN_USE;

    /* Assign file_op_table based on file type */
    switch (dentry.filetype)
    {
        case RTC_TYPE:
            fd_array[fd].file_ops = &rtc_type_op_table;
            break;
        case DIR_TYPE:
            fd_array[fd].file_ops = &dir_type_op_table;
            break;
        case FILE_TYPE:
            fd_array[fd].file_ops = &file_type_op_table;
            break;
        default:
            return FAILURE;
    }

    /* Call appropriate open function in fd's op table */
    if (fd_array[fd].file_ops->open == NULL || fd_array[fd].file_ops->open(filename) != SUCCESS)
        return FAILURE;

    return fd;
}

/*
 * system_close
 *   DESCRIPTION: Calls appropriate close function from the given FD's
 *                file_op_table. User is not allowed to close FDs 0 or 1
 *                (stdin/stdout).
 *        INPUTS: fd - file descriptor index
 *       OUTPUTS: none
 *  RETURN VALUE: Return value of file's close function or FAILURE for invalid
 *                FD or no close function.
 *  SIDE EFFECTS: Calls file's close function.
 */
int32_t system_close(int32_t fd)
{
    file_t* fd_array = get_current_pcb()->fd_table;

    /* Check for valid fd. FDs 0 and 1 should not be closed by user */
    if (fd < 2 || fd >= FD_ARRAY_SIZE || fd_array[fd].flags == NOT_IN_USE)
        return FAILURE;

    /* Call appropriate close function in fd's op table */
    if (fd_array[fd].file_ops->close != NULL)
        return fd_array[fd].file_ops->close(fd);

    return FAILURE;
}

/*
 * system_getargs
 *   DESCRIPTION: Copies the program's command line arguments into the given
 *                buffer.
 *        INPUTS: nbytes - number of bytes to copy
 *       OUTPUTS: buf - buffer that will hold the args
 *  RETURN VALUE: SUCCESS or FAILURE if arguments are longer than given buffer
 *                or given pointer is invalid.
 *  SIDE EFFECTS: none
 */
int32_t system_getargs(uint8_t* buf, int32_t nbytes)
{
    pcb_t* curr_pcb = get_current_pcb();

    /* Check if arguments are longer than given buffer and validate given pointer */
    if (curr_pcb->args_len > nbytes || __validate_user_ptr((uint32_t)buf) == FAILURE || curr_pcb->args_len == 0)
        return FAILURE;

    /* Copy arguments from PCB into buf */
    int i;
    for (i = 0; i < curr_pcb->args_len; ++i)
    {
        buf[i] = get_current_pcb()->args[i];
    }

    return SUCCESS;
}

/*
 * system_vidmap
 *   DESCRIPTION: Gives the virtual address of a user-accessable page which
 *                maps to video memory.
 *        INPUTS: none
 *       OUTPUTS: screen_start - pointer to a location in the user program in
 *                which to put the mapped user video page.
 *  RETURN VALUE: SUCCESS or FAILURE if screen_start is not valid
 *  SIDE EFFECTS: Maps a 4 KB user-accessable page to video memory.
 */
int32_t system_vidmap(uint8_t** screen_start)
{
    int32_t current_group = get_current_group();

    /* Validate that given pointer is in the user program page */
    if (__validate_user_ptr((uint32_t)screen_start) == FAILURE)
        return FAILURE;

    /* Mark as mapped */
    get_current_pcb()->vid_map_called = 1;

    /* Map user video page. User video page is mapped just below kernel video page */
    if (visible_group == current_group) {
        map_page(VIDEO_USER, VIDEO_KERNEL, TRUE, TRUE, FALSE);
    } else {
        map_page(VIDEO_USER, VIDEO_GROUP_1 + current_group * PAGE_SIZE, TRUE, TRUE, FALSE);
    }

    *screen_start = (uint8_t*)VIDEO_USER;

    return SUCCESS;
}

/*
 * __load_program
 *   DESCRIPTION: Opens given file, checks that it is an executable, then
 *                copies it into the program page. Closes file and returns the
 *                program's entry point (initial instruction pointer).
 *        INPUTS: filename - executable file to load
 *       OUTPUTS: none
 *  RETURN VALUE: Loaded program's IP or FAILURE if file fails to open or read
 *                or if file is not an executable.
 *  SIDE EFFECTS: Loads file into program page
 */
int32_t __load_program(const uint8_t* filename)
{
    /* Open file */
    int fd = system_open(filename);
    if (fd == FAILURE)
        return FAILURE;
    
    /* Check if file is executable by checking first 4 bytes */
    int magic_num;
    int ret = file_read(fd, (void*)&magic_num, EXEC_MAGIC_LEN);
    if (ret == FAILURE || ret < EXEC_MAGIC_LEN || magic_num != EXEC_MAGIC_STR)
        return FAILURE;

    /* Reset file position */
    get_current_pcb()->fd_table[fd].file_position = 0;

    /* Copy file into program page one byte at a time */
    uint32_t prog_addr = PROG_VIRT_ADDR + PROG_OFFSET;
    ret = file_read(fd, (void*)(prog_addr++), 1);
    while (ret != 0)
    {
        if (ret == FAILURE)
            return FAILURE;
        ret = file_read(fd, (void*)(prog_addr++), 1);
    }

    /* Close file */
    file_close(fd);

    /* If successful, return entry point of program */
    return *((uint32_t*)(PROG_VIRT_ADDR + PROG_OFFSET + PROG_EIP_OFF));
}

/*
 * get_prog_phys_addr
 *   DESCRIPTION: Gets physical address of a 4 MB block based on the program's
 *                PID.
 *        INPUTS: pid - proccess ID number
 *       OUTPUTS: none
 *  RETURN VALUE: Physical address of program block
 *  SIDE EFFECTS: none
 */
uint32_t get_prog_phys_addr(int32_t pid)
{
    return KERNEL_LOC_END + ((pid - 1) * PROG_PAGE_SIZE);
}

/*
 * __validate_user_ptr
 *   DESCRIPTION: Checks that given pointer is an address within the user
 *                program page.
 *        INPUTS: ptr - pointer to an object in the user program page
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS/FAILURE
 *  SIDE EFFECTS: none
 */
int32_t __validate_user_ptr(uint32_t ptr)
{
    if (ptr >= PROG_VIRT_ADDR && ptr < PROG_VIRT_ADDR + PROG_PAGE_SIZE)
        return SUCCESS;

    return FAILURE;
}

/* static_start_shell
 *   DESCRIPTION: Statically start shell 2 or 3. Sets up a fake stack to allow
 *                  for scheduling switches and standard execution.
 *        INPUTS: pid - 2 or 3 (for shell 2 or 3)
 *       OUTPUTS: N/A
 *  RETURN VALUE: SUCCESS or FAILURE
 *  SIDE EFFECTS: Sets up fake stack in kernel space for programs 2/3
 *                Updates active_pid array
 */
int32_t static_start_shell(int32_t pid)
{
    uint8_t filename[] = "shell";

    /* PID must be 2, or 3 */
    if (pid < 2 || pid > 3)
        return FAILURE;

    /* Create PCB */
    pcb_t* child_pcb = pcb_setup(pid);
    active_pid[0] = 0;  /* This happens before scheduling so active_pid[0] is accidentally overwritten */
    if (child_pcb == NULL)
        return FAILURE;

    /* Modify PCB's parent - should be 0 for kernel */
    child_pcb->parent_pid = 0;

    /* Starter shells have no arguments */
    child_pcb->args[0] = '\0';
    child_pcb->args_len = 0;

    /* Map program page */
    map_page(PROG_VIRT_ADDR, get_prog_phys_addr(child_pcb->pid), TRUE, TRUE, TRUE);

    /* Load program */
    uint32_t program_eip = __load_program(filename);
    if (program_eip == FAILURE) 
    {
        /* Clean up */
        map_page(PROG_VIRT_ADDR, get_prog_phys_addr(child_pcb->parent_pid), TRUE, TRUE, TRUE);
        pcb_teardown();
        return FAILURE;
    }

    /* Fill active pid number for scheduler  -- should happen after cleanup to
       override kernel default values */
    active_pid[pid - 1] = pid;

    /* Set EIP in PCB to entry point of program */
    child_pcb->eip = program_eip;

    /* Set ESP in PCB to a word (4 bytes) above the bottom of process page */
    child_pcb->esp = PROG_VIRT_ADDR + PROG_PAGE_SIZE - 4;

    /* Set tss_esp0 to be initial base of kernel stack */
    child_pcb->tss_esp0 = get_kstack_addr(child_pcb->pid);

    /* Set kernel esp and ebp in PCB to frame of schedule_next() (18 4-byte words above base of stack) */
    child_pcb->kernel_ebp = child_pcb->tss_esp0 - CHILD_EBP_OFF;
    child_pcb->kernel_esp = child_pcb->kernel_ebp - SIZE_VARS_SCHEDULING;

    /* Prep kernel stack with PIT interrupt context for scheduling */
    /* Push IRET context to "paused" execution:
     *  push xss
     *  push esp (start of user stack)
     *  push eflags and set IF to allow interrupts
     *  push xcs
     *  push return address (eip, entry point into program)
     * Push 12 (4 byte) sudo values for IRQ#, tss_esp0, regs(8), tss_esp0, IRQ#
     * Setup RET context for do_irq
     *  push return value (return_from_intr)
     *  push old ebp (base of kernel stack)
     */
    asm volatile(   
        "movl   %1, -4(%0)           \n\
         movl   %2, -8(%0)          \n\
         pushf                      \n\
         popl   %%eax               \n\
         orl    $0x200, %%eax       \n\
         movl   %%eax, -12(%0)       \n\
         movl   %3, -16(%0)         \n\
         movl   %4, -20(%0)         \n\
         movl   $return_from_intr, -64(%0) \n"
         :
         : "r" (child_pcb->tss_esp0),
           "r" (USER_DS),
           "r" (child_pcb->esp),
           "r" (USER_CS),
           "r" (child_pcb->eip)
         : "cc", "memory", "eax"
    );

    return SUCCESS;
}

