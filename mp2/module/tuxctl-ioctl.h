#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)

#define NUM_LEDS 4
#define DP_SHIFT 24
#define SHIFT_LEFT_4 4
#define CLEAR_LAST_BYTE 0xFFFFFF00
#define WHICH_LEDS_SHIFT 16
#define MASK_LAST_4 0xf
#define MASK_LAST_BIT 0x1
#define MASK_LAST_BYTE 0xff
#define DP_MASK 0x10
#define D_MASK 0x4
#define L_MASK 0x2
#define R00U_MASK 0x9
#define MAX_LED_BUFFER_SIZE 6
#define MIN_LED_BUFFER_SIZE 2
#define U_PRESSED 0x10
#define D_PRESSED 0x20
#define R_PRESSED 0x80
#define L_PRESSED 0x40
#define LED1 4
#define LED2 8
#define LED3 12
#define ALL_LEDS 0xf
#define RIGHTMOST_LEDS 0x7
#define DIV 10
#define LED2_DEC 0x4

#endif
