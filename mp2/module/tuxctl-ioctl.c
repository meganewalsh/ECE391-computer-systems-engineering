/*
 * tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

int ioctl_tux_init(struct tty_struct* tty);
int ioctl_tux_leds(struct tty_struct*, unsigned long arg);
int ioctl_tux_buttons(unsigned long arg);
void response_reset(struct tty_struct*);
void response_ack(void);
void response_bioc(unsigned, unsigned);

/* Variables to keep track of the state of the device  */
int ack = 0;
unsigned long LED_values;
unsigned packetC = 0, packetB = 0;
static spinlock_t l = SPIN_LOCK_UNLOCKED;	// for shared vars
static spinlock_t a = SPIN_LOCK_UNLOCKED;

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in
 * tuxctl-ld.c. It calls this function, so all warnings there apply
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet) {
    unsigned a, b, c;
    a = packet[0]; // holds opcode
    b = packet[1];
    c = packet[2];

    // Called on all RESPONSES we care about
    switch(a) {
	/* Generated when the device re-initializes itself after a power-up, a RESET button press, or an MTCP_RESET_DEV command.
	 * Will need to reestore controller state.
	 * 	Packet Format:
	 *      Byte 0 - MTCP_RESET
	 */
	case MTCP_RESET: {
		response_reset(tty);
		return;
	}

	/*  Response when the MTC successfully completes a command  */
	case MTCP_ACK: {
		response_ack();
		return;
	}

 	/*    Generated when the Button Interrupt-on-change mode is enabled and
 	*     a button is either pressed or released.
 	*     Packet format:
 	*        Byte 0 - MTCP_BIOC_EVENT
 	*        byte 1  __7____4___3___2___1_____0___
 	*            	| 1 X X X | C | B | A | START |
 	*            	-------------------------------
 	*        byte 2  +-7-----4-+---3---+---2--+--1---+--0-+
 	*                | 1 X X X | right | down | left | up |
 	*                +---------+-------+------+------+----+
 	*/
	case MTCP_BIOC_EVENT: {
		response_bioc(b, c);
		return;
	}
    }
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD.{
 * At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int tuxctl_ioctl(struct tty_struct* tty, struct file* file, unsigned cmd, unsigned long arg) {
    switch (cmd) {
        case TUX_INIT:
		return ioctl_tux_init(tty);
        case TUX_BUTTONS:
		/*  Check if pointer is invalid  */
		if (!arg)
			return -EINVAL;
		else
			return ioctl_tux_buttons(arg);
        case TUX_SET_LED:
		return ioctl_tux_leds(tty, arg);
        default:
            return -EINVAL;
    }
}

/*
 * ioctl_tux_init
 *   DESCRIPTION: Initializes any variables associated with the driver. Assume that
 *                any user-level code that interacts with your device will call this ioctl before any others
 *   INPUTS: tty (passed to put)
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: tux is ready for action
 */
int ioctl_tux_init(struct tty_struct* tty){
/*  MTCP_BIOC_ON : Enable Button interrupt-on-change. MTCP_ACK is returned.
 *  MTCP_LED_USR : Put the LED display into user-mode. In this mode, the value specified
 *    		   by the MTCP_LED_SET command is displayed.  */
    char buf[2] = {MTCP_LED_USR, MTCP_BIOC_ON};
    tuxctl_ldisc_put(tty, buf, 2);
    return 0;
}

/*
 * ioctl_tux_leds
 *   DESCRIPTION: Issues command to tux to diplay values on the leds 
 *   INPUTS:	tty
 *   		arg -- A 32-bit integer of the following form:
 *                  	bits 15-0	hex value to display (first two bytes)
 *                  	bits 19-16	which LEDs should be turned on (part of third byte)
 *                  	bits 27-24	whether corresponding decimal points should be turned on (part of fourth byte)
 *
 *                  		27-24		   19-16       15-0
 *                  +_______+__________+______ +____________+________+
 *                  |X|X|X|X| decimals |X|X|X|X| which leds |  value |
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: values on leds
 */
int ioctl_tux_leds(struct tty_struct* tty, unsigned long arg){
	char buf[MAX_LED_BUFFER_SIZE];
	int i, num, dp_loc, bufferSize = MIN_LED_BUFFER_SIZE, status;
/*          _A		
 *        F| |B		
 *          -G		Below is an array of the 16 possible numbers to be displayed
 *        E| |C		on the LEDs and their corresponding byte values, according to the
 *          -D .dp	illustrations shown here.
 *
 *      __7___6___5___4____3___2___1___0__
 *      | A | E | F | dp | G | C | B | D |
 *      +---+---+---+----+---+---+---+---+ */
	int ledHEX[16] =    {0xe7,		//  0x0 light up AEF CBD
			     0x06,		//  0x1 light up CB
			     0xcb,		//  0x2 light up AE GBD
		   	     0x8f,		//  0x3 light up A GCBD
			     0x2e,		//  0x4 light up F GCB
			     0xad,		//  0x5 light up AF GCD
			     0xed,		//  0x6 light up AEF GCD
			     0x86,		//  0x7 light up A CB
			     0xef,		//  0x8 light up AEF GCBD
			     0xae,		//  0x9 light up AF GCB
			     0xee,		//  0xa light up AEF GCB
			     0x6d,		//  0xb light up EF GCD
			     0xe1,		//  0xc light up AEF D
			     0x4f,		//  0xd light up E GCBD
			     0xe9,		//  0xe light up AEF GD
			     0xe8};		//  0xf light up AEF G

	/*  Save the values, used for reset later  */
    	spin_lock(&l);
	LED_values = arg;
    	spin_unlock(&l);
	
/*      __7___6___5___4____3______2______1______0___
 *      | X | X | X | X | LED3 | LED2 | LED1 | LED0 |
 *      ----+---+---+---+------+------+------+------+ */
	buf[0] = MTCP_LED_SET;
	buf[1] = ((arg >> WHICH_LEDS_SHIFT) & MASK_LAST_4);

	for (i = 0; i < NUM_LEDS; i++) {
		status = ((arg >> (WHICH_LEDS_SHIFT+i)) & MASK_LAST_BIT);
		if (status == 1) {
			num = ((arg >> (i*NUM_LEDS)) & MASK_LAST_4);
			buf[bufferSize] = ledHEX[num];
			/*  Account for decimal point  */
			dp_loc = ((arg >> (DP_SHIFT+i)) & MASK_LAST_BIT);
			if (dp_loc)
				buf[bufferSize] = buf[bufferSize] | DP_MASK;
			bufferSize++;
		}
	}
    	spin_lock(&a);
	if (ack){
		//printk("ack\n");
		ack = 0;
    		spin_unlock(&a);
		/*  Function declaration:
		 *  int tuxctl_ldisc_put(struct tty_struct *tty, char const *buf, int n)  */
		tuxctl_ldisc_put(tty, buf, bufferSize);
	}
	else
    		spin_unlock(&a);
return 0;
}

/*
 * ioctl_tux_buttons
 *   DESCRIPTION: Sets the bits of the low byte corresponding to the currently pressed buttons
 *   		  according to 
 *       	  	  +--7--+-6--+-5--+-4+3+2+1+--0--+
 *               	  |right|left|down|up|c|b|a|start|
 *                	  +-----+----+----+--+-+-+-+-----+
 *        b	__7_____4___3___2___1_____0___
 *            	| 1 X X X | C | B | A | START |
 *            	-------------------------------
 *        c  	+-7-----4-+---3---+---2--+--1---+--0-+
 *              | 1 X X X | right | down | left | up |
 *              +---------+-------+------+------+----+
 *   INPUTS: arg -- A pointer to a 32-bit integer.
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: changes last byte in arg
 */
int ioctl_tux_buttons(unsigned long arg){
	unsigned newByte, pc, pb; // keep local vars so ya don't need to keep locking
	unsigned long newArg;
	int d, l, r00u;

    	spin_lock(&l);
	pb = packetB;
	pc = packetC;
    	spin_unlock(&l);

	/*  Sets newByte = rldu0000. Need to fix the inconsistent direction ordering of c (rdlu) -> (rldu)  */
	d = ((pc & D_MASK) >> 1);		/*  0000_00d0  */
	l = ((pc & L_MASK) << 1);		/*  0000_0l00  */
	r00u = pc & R00U_MASK;			/*  0000_r00u  */
	newByte = (r00u | d | l) & MASK_LAST_4;
	newByte = newByte << SHIFT_LEFT_4;

	/*  Sets newByte = rlducbas  */
	newByte = (newByte | (pb & MASK_LAST_4));

	/*  Clears out last byte of arg as precaution
	 *  Side effect:  last byte is overwritten to 00 when no buttons are pressed  */
	newArg = (arg & CLEAR_LAST_BYTE) | newByte;

	/*  Function declaration:  copy_to_user(*to, *from, n)  */
	copy_to_user((unsigned long*)arg, &newArg, sizeof(newArg));

return 0;
}

/*  THE FOLLOWING FUNCS HANDLE RESPONSES by controlling the variables at the top of the file  */

/* response_reset
 *   DESCRIPTION: Handles when a reset is signaled from the tux
 *   INPUTS: tty
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Reinitializes the tux and restores the values on the leds
 */
void response_reset(struct tty_struct* tty){
	unsigned long vals;
	ioctl_tux_init(tty);
	/*  Now restore settings  */
    	spin_lock(&l);
	vals = LED_values;
    	spin_unlock(&l);
	ioctl_tux_leds(tty, vals);
	return;
}

/* response_ack
 *   DESCRIPTION: Handles when acknowledgment is signaled from the tux
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Signals that the task has been finished, signal used in setting leds
 */
void response_ack(){
    	spin_lock(&a);
	ack = 1;
    	spin_unlock(&a);
	return;
}

/* response_bioc
 *   DESCRIPTION: Handles when a button is pushed or released on the tux
 *   INPUTS: b -- packet B from handle_packet (cbas values)
 *   	     c -- packet C from handle_packed (rdlu values)
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: packetB and packetC are updated and switched to ACTIVE HIGH
 */
void response_bioc(unsigned b, unsigned c){
    	spin_lock(&l);
	packetC = (~c & MASK_LAST_4);
	packetB = (~b & MASK_LAST_4);
    	spin_unlock(&l);
	return;
}
