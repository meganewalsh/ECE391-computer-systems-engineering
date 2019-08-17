#include "keyboard.h"
#include "../types.h"
#include "../lib.h"
#include "../i8259.h"   /*TODO move i8259 to device/ folder */
#include "../term.h"

#define CMD_QUEUE_SIZE 50
#define inc_idx(idx) ((idx) = ((idx) + 1) % CMD_QUEUE_SIZE)
#define empty(start, end) ((start) == (end))
#define full(start, end) ((((end) + 1) % CMD_QUEUE_SIZE) == (start))
#define used(start, end) ((start) <= (end) ? ((end) - (start)) : (CMD_QUEUE_SIZE + (end) - (start)))
#define room(start, end) (CMD_QUEUE_SIZE - used(start,end) - 1)

#define NON_PRINTABLE -1

/* Local Helpers, see func def comments */
void __handle_interrupt(void);
uint8_t __send_cmd(uint8_t * cmd, uint8_t size);
void __send_head(void);
void __pop_head(void);
int8_t __get_character(uint8_t);  
uint8_t __is_alpha(char);
uint8_t __is_printable_ascii(char);

static uint8_t cmd_queue[CMD_QUEUE_SIZE];   /* Cyclic Command Queue, holds remaining bytes to write */
static unsigned start = 0;                  /* Head contain currently serviced command */
static unsigned end = 0;                   

/*static uint8_t scan_code_set = SET_SCAN_CODE_SET_1; TODO */
static uint8_t scan_code_extended = 0;              /* Whether or not the current code is extended */
static uint8_t scan_code_break = 0;                 /* Whether or not the current code is release */

/* Bottom 3 bits holds 
 * (bit 2) caps lock, 
 * (bit 1) num lock and 
 * (bit 0) scroll lock states */
static uint8_t lock_states = 0; 

/* Key State Buffer
 * Indexed by Keycodes listed in keyboard.h, 
 * if byte is 0, key is released
 * else, is pressed
 * */
static uint8_t keys[NUMBER_OF_KEYCODES];            


/* keyboard_init
 *  DESCRIPTION: Initialize the P/S2 Keyboard
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Enables interrupts on the keyboard and inits scan code sending.
 */
void keyboard_init() {
    uint8_t buff[3];
    enable_irq(KEY_IRQ);

    buff[0] = KEY_DISABLE_SCAN;
    __send_cmd(buff, 1);        /* Disable Scanning */

    buff[0] = KEY_ENABLE_SCAN;
    __send_cmd(buff, 1);        /* Enable Scanning */
}

/* keyboard_handler
 *  DESCRIPTION: Wrapper around the keyboard handler
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Reads byte from keyboard and updates key buffer (press/release)
 */
void keyboard_handler() {

    disable_irq(KEY_IRQ);
    send_eoi(KEY_IRQ);
    
    /*  Call handler  */
    __handle_interrupt();

    enable_irq(KEY_IRQ);
}

/* __handle_interrupt
 *  DESCRIPTION: Handle the interrupt and do all the side effects mentioned in keyboard_handler
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: See keyboard_handler()
 */
void __handle_interrupt() {
    unsigned long flags;
    uint8_t buff[3];             /* Local buffer to build any commands */
    uint32_t resp = inb(KEY_PORT); 
    unsigned int mapping = 0;
    int8_t c;

    switch(resp) {  
        case KEY_ACK:
                cli_and_save(flags);    /* Finished previous command */
                __pop_head();           /* Run next command if there is one in queue */
                __send_head();
                sti();
                restore_flags(flags);
            break;
        case KEY_RESEND:
                cli_and_save(flags);
                __send_head();          /* Resend last command */
                sti();
                restore_flags(flags);
            break;
        case KEY_ECHO:
            printf("ECHO Echo echo\n");
            break;
        /*case KEY_SELF_TEST_PASS: 
            break;
        case KEY_SELF_TEST_FAIL_1: 
            break;
        case KEY_SELF_TEST_FAIL_2: 
            break;
        case KEY_DETECTION_ERROR: 
            break;
        case KEY_DETECTION_ERROR_2:
            break;*/
        case SCAN_EXTENDED:
            scan_code_extended = 1;             
            break;
        default: 
            if (resp >= SET_1_RELEASE_OFFSET) {
                scan_code_break = 1;
                resp -= SET_1_RELEASE_OFFSET;
            }
            if (scan_code_extended) {
                /* Check extended section of scan map (second half) */
                mapping = scan_map[resp + MAP_START_OF_EXTENDED];
            } else {
                mapping = scan_map[resp];
            }
            if (scan_code_break) {
                /* Key is released */
                keys[mapping] = 0;
            } else {
                /* Key is pressed */
                keys[mapping] = 1;

                /* Handle specials i.e. locks */
                buff[0] = KEY_SET_LED;
                switch(mapping) {
                    case(KEY_SCROLL_LOCK):
                        lock_states ^= SCROLL_LOCK;    /* Toggle scroll lock using XOR */
                        buff[1] = lock_states;         /* Update LEDs on keyboard */
                        __send_cmd(buff, 2);
                        break;
                    case(KEY_CAPS_LOCK):
                        lock_states ^= CAPS_LOCK;    /* Toggle caps lock using XOR */
                        buff[1] = lock_states;      /* Update LEDs on keyboard */
                        __send_cmd(buff, 2);
                        break;
                    case(KEY_NUM_LOCK):
                        lock_states ^= NUM_LOCK;    /* Toggle num lock using XOR */
                        buff[1] = lock_states;      /* Update LEDs on keyboard */
                        __send_cmd(buff, 2);
                        break;
                    default: break;
                }


                c = __get_character(mapping);
                if (c != NON_PRINTABLE) {
                    add_char_term(c);
                } else if (mapping == KEY_BACKSPACE) {
                    c = '\b';
                    add_char_term(c);
                } else { /* Do nothing */ }

                if (keys[KEY_L] &&
                    (keys[KEY_LCTRL] || keys[KEY_RCTRL])) {
                    /* If CTRL-L is pressed */
                    clear_term();
                }

                if (keys[KEY_RALT_ORALTGR] || keys[KEY_LALT]) {
                    /* IF ALT+F# pressed, switch term */
                    if (keys[KEY_F1]) {
                        switch_term(0);
                    } else if (keys[KEY_F2]) {
                        switch_term(1);
                    } else if (keys[KEY_F3]) {
                        switch_term(2); /* TODO add macros */
                    }
                }
            }

            scan_code_extended = 0; /* Reset Extended/Break flags */
            scan_code_break = 0;
            break;
    }
}


/* __send_cmd
 *  DESCRIPTION: Queue command/run it if queue is empty
 *       INPUTS: cmd - bytes in command
 *               size - length of command
 *      OUTPUTS: None
 * RETURN VALUE: 0 on success, failure otherwise
 * SIDE EFFECTS: Update command queue and send out byte
 */
uint8_t __send_cmd(uint8_t * cmd, uint8_t size) {
    uint8_t ret = 0;
    long flags;
    uint8_t i = 0;
    cli_and_save(flags);
    if (empty(start, end)) {
        /* If queue is empty, send first byte of command before queue-ing */
        outb(cmd[0], KEY_PORT);   
    } 
    /* Then, queue command if there is space (if empty there will always be space) */
    if (room(start, end) > size) {
        for(i = 0; i < size; i++) {     /* Queue the cmd */
            cmd_queue[end] = cmd[i];
            inc_idx(end);
        }
    } else {
        ret = -1;
    }
    
    sti();
    restore_flags(flags);
    return ret;
}

/* __send_head
 *  DESCRIPTION: Sends the head of the queue to keyboard if it is not empty.
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: see description
 *
 * IMPORTANT: Assumes IF flag is cleared
 */
void __send_head(void) {
    if (!empty(start, end)) {
        outb(cmd_queue[0], KEY_PORT);
    }
}

/* __pop_head
 *  DESCRIPTION: Pops Command Byte at head of queue if not empty
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: see description
 *
 * IMPORTANT: Assumes IF flag is cleared
 */
void __pop_head(void) {
    if (!empty(start, end)) {
        inc_idx(start);
    }
}


/* __get_character
 *  DESCRIPTION: Returns the ASCII character associated with the key code
 *       INPUTS: code - Keycode from 0 to NUMBER_OF_KEYCODES (no error checking done here)
 *      OUTPUTS: None
 * RETURN VALUE: ASCII char or NON_PRINTABLE, if it is not printable
 * SIDE EFFECTS: None
 * IMPORTANT: No error checking is done on input so don't screw that up
 */
int8_t __get_character(uint8_t code) {
    int offset = 0; /* Offset into ascii lookup table */
    char ret = 0;

    if (code >= NUMBER_OF_KEYCODES || keys[KEY_LCTRL] || keys[KEY_RCTRL]) 
        return NON_PRINTABLE;

    if (keys[KEY_LSHIFT] || keys[KEY_RSHIFT]) { 
        /* If Shift is pressed */
        offset = NUMBER_OF_KEYCODES; 
    }
    ret = ascii_lookup[code + offset];

    if (__is_alpha(ret) && (lock_states & CAPS_LOCK)) {
        /* If caps lock, and is a-z/A-Z */
        offset = NUMBER_OF_KEYCODES;
        ret = ascii_lookup[code + offset];
    }

    if (__is_printable_ascii(ret))
        return ret;
    else
        return NON_PRINTABLE;
}


/* __is_alpha
 *  DESCRIPTION: Returns whether the input ASCII char is a-z or A-Z
 *       INPUTS: An ASCII char
 *      OUTPUTS: None
 * RETURN VALUE: true - is a-z or A-Z, false otherwise
 * SIDE EFFECTS: None
 */
uint8_t __is_alpha(char c) {
    return ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'));
}

/* __is_printable_ascii
 *  DESCRIPTION: Returns whether the input ASCII char is printable
 *       INPUTS: An ASCII char
 *      OUTPUTS: None
 * RETURN VALUE: true - is printable, false otherwise
 * SIDE EFFECTS: None
 */
uint8_t __is_printable_ascii(char c) {
    if (c == '\t' || c == '\n' || (c >= 32 && c <= 126)) {
        return 1 ;
    }
    return 0;
}
