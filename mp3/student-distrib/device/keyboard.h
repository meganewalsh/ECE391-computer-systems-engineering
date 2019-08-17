#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "../types.h"

#define KEY_IRQ     1
#define KEY_PORT    0x60   /* Keyboard port */

#define TERM_BUFFER_SIZE 128

/* Keyboard responses */
#define KEY_ACK                 0xFA
#define KEY_SELF_TEST_PASS      0xAA
#define KEY_SELF_TEST_FAIL_1    0xFC
#define KEY_SELF_TEST_FAIL_2    0xFD
#define KEY_RESEND              0xFE
#define KEY_DETECTION_ERROR     0xFF
#define KEY_DETECTION_ERROR_2   0x00


/* Keyboard Command Bytes */
#define KEY_SET_LED             0xED
#define KEY_ECHO                0xEE     /* Response is Echo or Resend */
#define KEY_SCAN_SET            0xF0
#define KEY_ID_BOARD            0xF2
#define KEY_SET_RATE            0xF3
#define KEY_ENABLE_SCAN         0xF4
#define KEY_DISABLE_SCAN        0xF5
#define KEY_SET_DEFAULT         0xF6
#define KEY_SET_AUTO_REP        0xF7
#define KEY_SET_MAKE_REL        0xF8
#define KEY_SET_MAKE_O          0xF9
#define KEY_SET_AUTO_MAKE_OREL  0xFA
#define KEY_SET_KEY_AUTO        0xFB
#define KEY_SET_KEY_MAKE_REL    0xFC
#define KEY_SET_KEY_MAKE_O      0xFD
#define KEY_RESEND_LAST         0xFE
#define KEY_RESET               0xFF

#define SCROLL_LOCK 0x01    /* Bits in Data Byte for SET_LED */
#define NUM_LOCK    0x02
#define CAPS_LOCK   0x04

#define GET_SCAN_CODE       0
#define SET_SCAN_CODE_SET_1 1
#define SET_SCAN_CODE_SET_2 2
#define SET_SCAN_CODE_SET_3 3

/* Define Key Code Mappings */
/* KEY stands for Key Code */
#define NUMBER_OF_KEYCODES 125
#define KEY_ESCAPE 0
#define KEY_1 1
#define KEY_2 2
#define KEY_3 3
#define KEY_4 4
#define KEY_5 5
#define KEY_6 6
#define KEY_7 7
#define KEY_8 8
#define KEY_9 9
#define KEY_0 10
#define KEY_MINUS 11
#define KEY_EQUAL 12
#define KEY_BACKSPACE 13
#define KEY_TAB 14
#define KEY_Q 15
#define KEY_W 16
#define KEY_E 17
#define KEY_R 18
#define KEY_T 19
#define KEY_Y 20
#define KEY_U 21
#define KEY_I 22
#define KEY_O 23
#define KEY_P 24
#define KEY_OPEN_BRACKET 25
#define KEY_CLOSE_BRACKET 26
#define KEY_ENTER 27
#define KEY_LCTRL 28
#define KEY_A 29
#define KEY_S 30
#define KEY_D 31
#define KEY_F 32
#define KEY_G 33
#define KEY_H 34
#define KEY_J 35
#define KEY_K 36
#define KEY_L 37
#define KEY_SEMI_COLON 38
#define KEY_APOSTROPHE 39
#define KEY_BACKTICK 40
#define KEY_LSHIFT 41
#define KEY_BACKSLASH 42
#define KEY_Z 43
#define KEY_X 44
#define KEY_C 45
#define KEY_V 46
#define KEY_B 47
#define KEY_N 48
#define KEY_M 49
#define KEY_COMMA 50
#define KEY_PERIOD 51
#define KEY_SLASH 52
#define KEY_RSHIFT 53
#define KEY_PAD_STAR 54
#define KEY_LALT 55
#define KEY_SPACE 56
#define KEY_CAPS_LOCK 57
#define KEY_F1 58
#define KEY_F2 59
#define KEY_F3 60
#define KEY_F4 61
#define KEY_F5 62
#define KEY_F6 63
#define KEY_F7 64
#define KEY_F8 65
#define KEY_F9 66
#define KEY_F10 67
#define KEY_NUM_LOCK 68
#define KEY_SCROLL_LOCK 69
#define KEY_PAD_7 70
#define KEY_PAD_8 71
#define KEY_PAD_9 72
#define KEY_PAD_MINUS 73
#define KEY_PAD_4 74
#define KEY_PAD_5 75
#define KEY_PAD_6 76
#define KEY_PAD_PLUS 77
#define KEY_PAD_1 78
#define KEY_PAD_2 79
#define KEY_PAD_3 80
#define KEY_PAD_0 81
#define KEY_PAD_PERIOD 82
#define KEY_F11 83
#define KEY_F12 84
#define KEY_MULT_PREVIOUSTRACK 85
#define KEY_MULT_NEXTTRACK 86
#define KEY_PAD_ENTER 87
#define KEY_RCTRL 88
#define KEY_MULT_MUTE 89
#define KEY_MULT_CALCULATOR 90
#define KEY_MULT_PLAY 91
#define KEY_MULT_STOP 92
#define KEY_PRTSC 93
#define KEY_MULT_VOLUMEDOWN 94
#define KEY_MULT_VOLUMEUP 95
#define KEY_MULT_WWWHOME 96
#define KEY_PAD_SLASH 97
/*#define KEY_PRTSC 98 already defined above */
#define KEY_RALT_ORALTGR 99
#define KEY_HOME 100
#define KEY_CURSORUP 101
#define KEY_PAGEUP 102
#define KEY_CURSORLEFT 103
#define KEY_CURSORR 104
#define KEY_END 105
#define KEY_CURSORDOWN 106
#define KEY_PAGEDOWN 107
#define KEY_INSERT 108
#define KEY_DELETE 109
#define KEY_LEFTGUI 110
#define KEY_RGUI 111
#define KEY_APPS 112
#define KEY_ACPI_POWER 113
#define KEY_ACPI_SLEEP 114
#define KEY_ACPI_WAKE 115
#define KEY_MULT_WWWSEARCH 116
#define KEY_MULT_WWWFAVORITES 117
#define KEY_MULT_WWWREFRESH 118
#define KEY_MULT_WWWSTOP 119
#define KEY_MULT_WWWFORWARD 120
#define KEY_MULT_WWWBACK 121
#define KEY_MULT_MYCOMPUTER 122
#define KEY_MULT_EMAIL 123
#define KEY_MULT_MEDIASELECT 124

#define MAX_SCANCODE_SIZE 6 

#define SCAN_EXTENDED   0xE0 
#define SCAN_BREAK      0xF0
#define SCAN_PAUSE_KEY  0xE1

#define KEY_MAP_SIZE 512
#define MAP_START_OF_EXTENDED   256 /* Index in map at which point extended codes start */
#define SET_1_RELEASE_OFFSET    0x80    /* Subtract this from scan codes that are larger than 0x80 to get index of item*/

/* Maps Scan Code Set 1 to Key Codes
 * For codes above SET_1_RELEASE_OFFSET, subtract  SET_1_RELEASE_OFFSET to get idx into map
 */
static const uint8_t scan_map[KEY_MAP_SIZE] = { /* Scan Code Set 1 */
    0, KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,
    KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_OPEN_BRACKET, KEY_CLOSE_BRACKET, KEY_ENTER, KEY_LCTRL, KEY_A, KEY_S,
    KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMI_COLON, KEY_APOSTROPHE, KEY_BACKTICK, KEY_LSHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V,
    KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_RSHIFT, KEY_PAD_STAR, KEY_LALT, KEY_SPACE, KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUM_LOCK, KEY_SCROLL_LOCK, KEY_PAD_7, KEY_PAD_8, KEY_PAD_9, KEY_PAD_MINUS, KEY_PAD_4, KEY_PAD_5, KEY_PAD_6, KEY_PAD_PLUS, KEY_PAD_1,
    KEY_PAD_2, KEY_PAD_3, KEY_PAD_0, KEY_PAD_PERIOD, 0, 0, 0, KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*Extended Scan Codes */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    KEY_MULT_PREVIOUSTRACK, 0, 0, 0, 0, 0, 0, 0, 0, KEY_MULT_NEXTTRACK, 0, 0, KEY_PAD_ENTER, KEY_RCTRL, 0, 0,
    KEY_MULT_MUTE, KEY_MULT_CALCULATOR, KEY_MULT_PLAY, 0, KEY_MULT_STOP, 0, 0, 0, 0, 0, KEY_PRTSC, 0, 0, 0, KEY_MULT_VOLUMEDOWN, 0,
    KEY_MULT_VOLUMEUP, 0, KEY_MULT_WWWHOME, 0, 0, KEY_PAD_SLASH, 0, KEY_PRTSC, KEY_RALT_ORALTGR, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, KEY_HOME, KEY_CURSORUP, KEY_PAGEUP, 0, KEY_CURSORLEFT, 0, KEY_CURSORR, 0, KEY_END,
    KEY_CURSORDOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, 0, 0, 0, 0, 0, 0, 0, KEY_LEFTGUI, KEY_RGUI, KEY_APPS, KEY_ACPI_POWER, KEY_ACPI_SLEEP,
    0, 0, 0, KEY_ACPI_WAKE, 0, KEY_MULT_WWWSEARCH, KEY_MULT_WWWFAVORITES, KEY_MULT_WWWREFRESH, KEY_MULT_WWWSTOP, KEY_MULT_WWWFORWARD, KEY_MULT_WWWBACK, KEY_MULT_MYCOMPUTER, KEY_MULT_EMAIL, KEY_MULT_MEDIASELECT, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Contains ASCII values for keys where applicable, 0x00 else 
 * Second half of table contains values on SHIFT-press
 * Indexed by Key Code Mappings above
 */
static const char ascii_lookup[2 * NUMBER_OF_KEYCODES] = {
    0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x08, '\t', 'q',
    'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x00, 'a', 's', 'd',
    'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x00, '\\', 'z', 'x', 'c', 'v', 'b',
    'n', 'm', ',', '.', '/', 0x00, '*', 0x00, ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2',
    '3', '0', '.', 0x00, 0x00, 0x00, 0x00, '\n', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '/', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* ASCII Characters when "SHIFT" is pressed */ 
    0X1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0X08, '\t', 'Q',
    'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0X00, 'A', 'S', 'D',
    'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0X00, '|', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', '<', '>', '?', 0X00, '*', 0X00, ' ', 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2',
    '3', '0', '.', 0X00, 0X00, 0X00, 0X00, '\n', 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00,
    0X00, '/', 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X7F, 0X00, 0X00,
    0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00
};

/* TODO handle 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77 pause pressed */

void keyboard_init(void);
void keyboard_handler(); 


int32_t term_read (int32_t fd, void* buf, int32_t nbytes);
int32_t term_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t term_open(const uint8_t* filename);
int32_t term_close(int32_t fd);

#endif
