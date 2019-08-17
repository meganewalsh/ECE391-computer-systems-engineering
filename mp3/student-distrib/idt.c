#include "x86_desc.h"
#include "idt.h"
#include "lib.h"
#include "device/keyboard.h"
#include "rtc.h"
#include "system.h"
#include "scheduler.h"
#include "pit.h"

/*
 * set_idt_interrupt_gate
 *   DESCRIPTION: Fill in IDT entry
 *        INPUTS: str - pointer to the entry struct
 *                d - size
 *                priv_level - privilege level
 *                p - present
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
#define SET_IDT_INTERUPT_GATE(str, d, priv_level, p)    \
do {                                                    \
    str.reserved4 = 0;                                  \
    str.reserved3 = 0;                                  \
    str.reserved2 = 1;                                  \
    str.reserved1 = 1;                                  \
    str.size = d;                                       \
    str.reserved0 = 0;                                  \
    str.dpl = priv_level;                               \
    str.present = p;                                    \
    str.seg_selector = KERNEL_CS;                       \
} while (0)

/*
 * set_idt_trap_gate
 *   DESCRIPTION: Fill in IDT entry
 *        INPUTS: str - pointer to the entry struct
 *                d - size
 *                priv_level - privilege level
 *                p - present
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
#define SET_IDT_TRAP_GATE(str, d, priv_level, p)        \
do {                                                    \
    str.reserved4 = 0;                                  \
    str.reserved3 = 1;                                  \
    str.reserved2 = 1;                                  \
    str.reserved1 = 1;                                  \
    str.size = d;                                       \
    str.reserved0 = 0;                                  \
    str.dpl = priv_level;                               \
    str.present = p;                                    \
    str.seg_selector = KERNEL_CS;                       \
} while (0)

/* Exception code strings */
char* ExceptionCode[NUM_EXCEPTIONS] = {
    "Division Error",                   // fault
    "Debug Exception",                  // trap or fault
    "NMI Interrupt",                    // save state of processor, then handle interrupt and restore processor
    "Breakpoint Exception",             // trap
    "Overflow Exception",               // trap
    "Bound Range Exceeded Expectation", // fault
    "Invalid Opcode Execution",         // fault, no program state change
    "Device Not Available Exception",   // fault
    "Double Fault Exception",           // abort, push 0, cannot be returned
    "Coprocessor Segment Overrun",      // abort
    "TSS Exception",                    // fault
    "Segment Not Present",              // fault
    "Stack Fault Exception",            // fault
    "General Protection Exception",     // fault
    "Page Fault Exception",             // fault
    "Assertion Exception",              // INTEL RESERVED
    "FPU Floating Point Error",         // fault
    "Alignment Check Exception",        // fault, exception error code yes(always 0)
    "Machine Check Exception",          // abort
    "SIMD Floating Point Exception"     // fault
};


/***** Extended IRET functionality  *****/

asm(
    ".global iret_and_save_tss_esp  \n\
     iret_and_save_tss_esp:         \n\
        pushl   %eax                \n\
        pushl   %ebx                \n\
        pushl   %ecx                \n\
        movl    %esp, %ecx          \n\
        addl    $12, %ecx           \n\
        leal    tss, %eax           \n\
        addl    $4, %eax            \n\
        movl    4(%ecx), %ebx       \n\
        andl    $0x03, %ebx         \n\
        cmpl    $0, %ebx            \n\
        jne     _cpl_is_not_zero    \n\
        addl    $12, %ecx           \n\
        jmp     _load_tss           \n\
     _cpl_is_not_zero:              \n\
        addl    $20, %ecx           \n\
     _load_tss:                     \n\
        movl    %ecx, (%eax)        \n\
        popl    %ecx                \n\
        popl    %ebx                \n\
        popl    %eax                \n\
        iret                        \n "
);

/***** Exception Handling *****/

/* Return from Exception
 *  Restores all regs, pops exc number, and IRET
 */
asm
(
    "return_from_exc:                   \n\
        popal                           \n\
        addl    $4, %esp                \n\
        jmp     iret_and_save_tss_esp   \n"
);

/* Common Exception
 *  Saves all regs, pushes exc number as parameter to do_exc, calls do_exc,
 *  pops exc number, and jumps to return from exception.
 */
asm
(
    "common_exc:                    \n\
        pushal                      \n\
        cld                         \n\
        pushl   32(%esp)            \n\
        call    do_exc              \n\
        addl    $4, %esp            \n\
        jmp     return_from_exc     \n"
);

/* Exception IDT Stubs (0-19)
 *  Pushes exception number and calls common_exc.
 */
asm
(
    "exc00:                         \n\
        pushl   $0                  \n\
        jmp     common_exc          \n\
    exc01:                          \n\
        pushl   $1                  \n\
        jmp     common_exc          \n\
    exc02:                          \n\
        pushl   $2                  \n\
        jmp     common_exc          \n\
    exc03:                          \n\
        pushl   $3                  \n\
        jmp     common_exc          \n\
    exc04:                          \n\
        pushl   $4                  \n\
        jmp     common_exc          \n\
    exc05:                          \n\
        pushl   $5                  \n\
        jmp     common_exc          \n\
    exc06:                          \n\
        pushl   $6                  \n\
        jmp     common_exc          \n\
    exc07:                          \n\
        pushl   $7                  \n\
        jmp     common_exc          \n\
    exc08:                          \n\
        pushl   $8                  \n\
        jmp     common_exc          \n\
    exc09:                          \n\
        pushl   $9                  \n\
        jmp     common_exc          \n\
    exc0A:                          \n\
        pushl   $10                 \n\
        jmp     common_exc          \n\
    exc0B:                          \n\
        pushl   $11                 \n\
        jmp     common_exc          \n\
    exc0C:                          \n\
        pushl   $12                 \n\
        jmp     common_exc          \n\
    exc0D:                          \n\
        pushl   $13                 \n\
        jmp     common_exc          \n\
    exc0E:                          \n\
        pushl   $14                 \n\
        jmp     common_exc          \n\
    exc0F:                          \n\
        pushl   $15                 \n\
        jmp     common_exc          \n\
    exc10:                          \n\
        pushl   $16                 \n\
        jmp     common_exc          \n\
    exc11:                          \n\
        pushl   $17                 \n\
        jmp     common_exc          \n\
    exc12:                          \n\
        pushl   $18                 \n\
        jmp     common_exc          \n\
    exc13:                          \n\
        pushl   $19                 \n\
        jmp     common_exc          \n"
);

/* List of exception stub labels so easily accessable in C code */
static void (*exc_stub_labels[NUM_EXCEPTIONS]) = { exc00, exc01, exc02, exc03, exc04, exc05, exc06, exc07, exc08, exc09, exc0A, exc0B, exc0C, exc0D, exc0E, exc0F, exc10, exc11, exc12, exc13 };

/*
 * do_exc
 *   DESCRIPTION: Prints exception number and string. Called by common
 *                exception assembly.
 *        INPUTS: exc_number - interrupt exc number, passed through assembly
 *                push
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Never returns
 */
void do_exc(int exc_number) {
    /* Print Exception Number/Info */
    printf("EXCEPTION %d: %s\n", exc_number, ExceptionCode[exc_number]);

    sti();
    system_halt(HALT_CODE_EXC);
}


/***** Interrupt Handling *****/

/* Return from Interrupt
 *  Restores all regs, pops IRQ number, and IRET
 */
asm
(
    ".global return_from_intr           \n\
    return_from_intr:                   \n\
        popal                           \n\
        addl    $8, %esp                \n\
        jmp     iret_and_save_tss_esp   \n"
);

/* Common Interrupt
 *  Pushes the tss_esp0 saves all regs, pushes parameters to
 *  do_IRQ, calls do_IRQ, pops IRQ number, and jumps to return from interrupt.
 */
asm
(
    "common_interrupt:              \n\
        pushl   %esp                \n\
        addl    $4, (%esp)          \n\
        pushal                      \n\
        cld                         \n\
        pushl   44(%esp)            \n\
        pushl   36(%esp)            \n\
        pushl   44(%esp)            \n\
        call    do_irq              \n\
        addl    $12, %esp           \n\
        jmp     return_from_intr    \n"
);

/* Interrupt IDT Stubs (0-15)
 *  Pushes interrupt IRQ number and calls common_interrupt.
 */
asm
(
    "irq0:                          \n\
        pushl   $0                  \n\
        jmp     common_interrupt    \n\
    irq1:                           \n\
        pushl   $1                  \n\
        jmp     common_interrupt    \n\
    irq2:                           \n\
        pushl   $2                  \n\
        jmp     common_interrupt    \n\
    irq3:                           \n\
        pushl   $3                  \n\
        jmp     common_interrupt    \n\
    irq4:                           \n\
        pushl   $4                  \n\
        jmp     common_interrupt    \n\
    irq5:                           \n\
        pushl   $5                  \n\
        jmp     common_interrupt    \n\
    irq6:                           \n\
        pushl   $6                  \n\
        jmp     common_interrupt    \n\
    irq7:                           \n\
        pushl   $7                  \n\
        jmp     common_interrupt    \n\
    irq8:                           \n\
        pushl   $8                  \n\
        jmp     common_interrupt    \n\
    irq9:                           \n\
        pushl   $9                  \n\
        jmp     common_interrupt    \n\
    irqA:                           \n\
        pushl   $10                 \n\
        jmp     common_interrupt    \n\
    irqB:                           \n\
        pushl   $11                 \n\
        jmp     common_interrupt    \n\
    irqC:                           \n\
        pushl   $12                 \n\
        jmp     common_interrupt    \n\
    irqD:                           \n\
        pushl   $13                 \n\
        jmp     common_interrupt    \n\
    irqE:                           \n\
        pushl   $14                 \n\
        jmp     common_interrupt    \n\
    irqF:                           \n\
        pushl   $15                 \n\
        jmp     common_interrupt    \n"
);

/* List of interrupt stub labels so easily accessable in C code */
static void (*int_stub_labels[NUM_INTERRUPTS]) = { irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7, irq8, irq9, irqA, irqB, irqC, irqD, irqE, irqF };

/*
 * do_irq
 *   DESCRIPTION: Calls appropriate interrupt handler based on given irq
 *                number. Called by common interrupt assembly.
 *        INPUTS: irq_number - interrupt IRQ number, passed through assembly
 *                proc_push_top - top of process' stack, used for scheduling
                  pushed_cs - code segment register, used for scheduling
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Calls interrupt handlers
 */
void do_irq(int irq_number, uint32_t proc_push_top, uint32_t pushed_cs)
{
    switch (irq_number)
    {
        case IRQ_KB:
            keyboard_handler();
            break;
        case IRQ_RTC:
            rtc_wrapper();
            break;
        case IRQ_PIT:
            schedule_next(proc_push_top, pushed_cs);
            break;

        default:
            break;
    }
}

/*
 * +-31--16-+-15-+-14-13-+-12------8-+-7---5-+-4------0-+
 * | Offset | P  |  DPL  | 0 D 1 1 T | 0 0 0 | reserved |
 * +--------+----+-------+-----------+-------+----------+
 *   D - size of gate: 1 = 32 bits; 0 = 16 bits
 *   T - 1 = trap gate; 0 = interrupt gate
 * 
 * +-31------------16-+-15---0-+
 * | Segment Selector | Offset |
 * +------------------+--------+
 */
/*
 * set_all_idt
 *   DESCRIPTION: Fills in entries of IDT
 *        INPUTS: idt - pointer to the IDT
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
void set_all_idt(idt_desc_t* idt)
{
    int i;

    /* For each entry in the IDT */
    for (i = 0; i < NUM_VEC; i++)
    {
        /* Point gates to exception stubs */
        if (i <= IDT_EXC_19)
        {
            SET_IDT_ENTRY(idt[i], exc_stub_labels[i]);
            SET_IDT_INTERUPT_GATE(idt[i], 1, 0, 1);
        }
        /* Point gates to interrupt stubs */
        else if (i >= IDT_INT_0 && i <= IDT_INT_F)
        {
            SET_IDT_ENTRY(idt[i], int_stub_labels[i - IDT_INT_0]);
            SET_IDT_INTERUPT_GATE(idt[i], 1, 0, 1);
        }
        /* Point gate to sys call handler */
        else if (i == IDT_SYS_CALL)
        {
            SET_IDT_ENTRY(idt[i], system_call_handler);
            SET_IDT_TRAP_GATE(idt[i], 1, 3, 1);
        }
    }
}

