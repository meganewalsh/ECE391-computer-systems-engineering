#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "tests.h"
#include "pcb.h"
#include "file.h"
#include "scheduler.h"

#define RTC_FREQ        1024
#define RTC_MAX_DIVIDER 6
#define RTC_OFFSET      4
#define RTC_WAITING     1
#define RTC_NOT_WAITING 0

/* Set to 1 when interrupt is caught, reset to 0 on read() */
static volatile int rtc_read_waiting[3];
static volatile int rtc_intr_count[3];
static volatile int rtc_freq_divider[3];

/*
 * rtc_init
 *   DESCRIPTION: Initializes register A and B of CMOS to allow RTC interrupts.
 *        Code and concepts taken from https://wiki.osdev.org/RTC
 *    INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 */
void rtc_init()
{
    int flags;
    cli_and_save(flags);

    /* Initialize rtc type file_op_table */
    rtc_type_op_table.read = rtc_read;
    rtc_type_op_table.write = rtc_write;
    rtc_type_op_table.open = rtc_open;
    rtc_type_op_table.close = rtc_close;

    /* Turn on periodic interrupt enable */
    outb(RTC_REG_B_NMI, RTC_PORT0);     /* Select register B 0x0B and disable NMI 0x80 */
    char prevB = inb(RTC_PORT1);        /* Current value of register B */
    outb(RTC_REG_B_NMI, RTC_PORT0);     /* Reset index to B */
    outb(prevB | 0x40, RTC_PORT1);      /* Turn on bit 6 of register B (Periodic Interrupt Enable) */

    #if RUN_TESTS
    rtc_count = 0;
    #endif

    sti();

    /* Enable RTC Interrupts */
    enable_irq(IRQ_8);
}

/*
 * rtc_wrapper
 *   DESCRIPTION: Interrupt handler for RTC
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
void rtc_wrapper(void)
{
    /* Mask interrupts until handled */
    disable_irq(IRQ_8);
    send_eoi(IRQ_8);

    /* For each process group */
    int group;
    for (group = 0; group < MAX_PROCESS_GROUPS; ++group)
    {
        if (rtc_read_waiting[group] == 1)
        {
            /* Count number of interrupts seen */
            ++rtc_intr_count[group];
            
            /* If the correct number of RTC interrupts has been seen, mark as not waiting */
            if (rtc_intr_count[group] >= RTC_FREQ/rtc_freq_divider[group])
            {
                rtc_read_waiting[group] = RTC_NOT_WAITING;
            }
        }
    }

    /* Read/Ignore C register, needed to receive another interrupt */
    outb(RTC_REG_C, RTC_PORT0);
    inb(RTC_PORT1);

    /* Re-enable interrupts */
    enable_irq(IRQ_8);
}

/*
 * rtc_read
 *   DESCRIPTION: Returns only after an interrupt has occurred.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: 0
 *  SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    /* Void Variables in order to keep the signatures similar*/
    (void) fd;
    (void) buf;
    (void) nbytes;

    #if RUN_TESTS
    tests_rtc_read_waited_for_int = 0;
    #endif

    int group = get_current_group();

    /* Set waiting flag */
    rtc_read_waiting[group] = RTC_WAITING;

    /* Clear count */
    rtc_intr_count[group] = 0;

    /* Wait for interrupt to occur before continuing */
    while (rtc_read_waiting[group] != RTC_NOT_WAITING){};

    #if RUN_TESTS
    tests_rtc_read_waited_for_int = 1;
    #endif

    return SUCCESS;
}


/* 
 * rtc_write
 *   DESCRIPTION: Sets the rate of RTC periodic interrupts according to the input. 
 *                Blocks interrupts while writing.
 *        INPUTS: buf -- A 4-byte integer specifying the interrupt rate in Hz
 *                max of 1024Hz
 *       OUTPUTS: none
 *  RETURN VALUE: 0
 *  SIDE EFFECTS: Changes the frequency of the RTC interrupts if valid
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    (void) fd;
    (void) nbytes;

    int flags;
    cli_and_save(flags);

    int32_t input = *(int32_t*) buf;

    /* Checks a power of 2 and within bounds, return 0 */
    if ((input & (input-1)) == 0 && (input >= MIN_RTC_RATE) && (input <= MAX_RTC_RATE)) {

        int group = get_current_group();

        rtc_freq_divider[group] = input * RTC_OFFSET;

        // /* Determine new rate corresponding to which power of 2 */
        // int8_t new_rate = 0;
        // while (input != 0x01) {
        //     new_rate++;
        //     input = input >> 1;
        // }
        // new_rate = 16 - new_rate;           /* Note: new_rate is capped at 15 since input <= 2^15 */

        // /* Change interrupt frequency. Rate value must be between 1 and 15 */
        // outb(RTC_REG_A_NMI, RTC_PORT0);     /* Select register A 0x0A and disable NMI 0x80 */
        // char prevA = inb(RTC_PORT1);        /* Current value of register A */
        // prevA = prevA & 0xF0;               /* Clear out previous rate to overwrite with new */
        // outb(RTC_REG_A_NMI, RTC_PORT0);     /* Reset index to A */
        // outb(prevA | new_rate, RTC_PORT1);  /* Write rate to A */

        sti();
        return SUCCESS;
    } else {
        /* Not a power of 2, do nothing and return -1 */
        sti();
        return FAILURE;
    }
}

/*
 * rtc_open
 *   DESCRIPTION: Initializes RTC frequency to 2 Hz.
 *        INPUTS: filename - must be "rtc"
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS
 *  SIDE EFFECTS: Changes the frequency of the RTC interrupts
 */
int32_t rtc_open(const uint8_t* filename)
{
    /* Change interrupt frequency to 2 Hz<-->15 */
    outb(RTC_REG_A_NMI, RTC_PORT0);         /* Select register A 0x0A and disable NMI 0x80 */
    char prevA = inb(RTC_PORT1);            /* Current value of register A */
    prevA = prevA & 0xF0;                   /* Clear out previous rate to overwrite with new */
    outb(RTC_REG_A_NMI, RTC_PORT0);         /* Reset index to A */
    // outb(prevA | RTC_RATE_2Hz, RTC_PORT1);  /* Write rate to A */
    outb(prevA | RTC_MAX_DIVIDER, RTC_PORT1);  /* Write rate to A */

    int group = get_current_group();
    rtc_freq_divider[group] = 2;
    rtc_read_waiting[group] = 0;
    rtc_intr_count[group] = 0;

    return SUCCESS;
}

/*
 * rtc_close
 *   DESCRIPTION: Marks the given file descriptor as not in use.
 *        INPUTS: fd - file descriptor index of file
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS/FAILURE
 *  SIDE EFFECTS: Clears file descriptor
 */
int32_t rtc_close(int32_t fd)
{
    /* Check for valid fd */
    if (fd < 0 || fd >= FD_ARRAY_SIZE)
        return FAILURE;

    /* Mark file descriptor as not in use */
    file_t* fd_array = get_current_pcb()->fd_table;
    fd_array[fd].file_ops = NULL;
    fd_array[fd].flags = NOT_IN_USE;

    return SUCCESS;
}

