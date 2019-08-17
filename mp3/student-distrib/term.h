#ifndef TERM_H_
#define TERM_H_

#include "types.h"
#include "file.h"
#include "pcb.h"

#define TERM_BUFFER_SIZE    128
#define TAB_SIZE            4

file_op_table_t stdin_op_table;
file_op_table_t stdout_op_table;

typedef struct term_struct {
    volatile uint8_t read_in_progress;
    volatile uint8_t newline_seen;
    uint8_t term_buff[TERM_BUFFER_SIZE]; /* Single Buffer, flushed by \n */
    unsigned term_buff_size; 
    int cursor_x;
    int cursor_y;
} term_struct_t;

void term_init();

int32_t term_read(int32_t fd, void* buf, int32_t nbytes);
int32_t term_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t term_open(const uint8_t* filename);
int32_t term_close(int32_t fd);

void clear_term(void);
int8_t add_char_term(uint8_t c);

int32_t switch_term(int32_t group_num);

#endif
