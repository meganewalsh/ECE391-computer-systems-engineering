
#include "term.h"
#include "types.h"
#include "lib.h"
#include "pcb.h"
#include "paging.h"
#include "scheduler.h"

/* Local Helpers, see func def comments */
int8_t __add_char_to_term(uint8_t);
void __backspace_term(void);
void __print_char(char);
uint32_t get_video_save_page(int32_t);

/* Save states for working terminals */
static term_struct_t terms[MAX_PROCESS_GROUPS];

/* term_init 
 *  DESCRIPTION: Initializes stdin/stdout file op tables, clears out terms,
                 sets current terminal to be 0
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: None
 */
void term_init()
{
    int i;

    /* Initialize stdin file_op_table */
    stdin_op_table.read = term_read;
    stdin_op_table.write = NULL;
    stdin_op_table.open = term_open;
    stdin_op_table.close = term_close;

    /* Initialize stdout file_op_table */
    stdout_op_table.read = NULL;
    stdout_op_table.write = term_write;
    stdout_op_table.open = term_open;
    stdout_op_table.close = term_close;

    /* Initialize terms and set current term to 0 */
    for (i = 0; i < MAX_PROCESS_GROUPS; i++) {
        terms[i].read_in_progress = 0;
        terms[i].newline_seen = 0;
        terms[i].term_buff_size = 0; 
        terms[i].cursor_x = 0;
        terms[i].cursor_y = 0;
    }

    visible_group = 0;
}

/* term_read 
 *  DESCRIPTION: Reads line-buffered into from kbd into buf
 *       INPUTS: fd - not used
 *               buf - char ptr in which to store buffered input
 *                     size must be at least TERM_BUFFER_SIZE
 *               nbytes - not used
 *      OUTPUTS: None
 * RETURN VALUE: Number of bytes copied to buf
 * SIDE EFFECTS: None
 */
int32_t term_read(int32_t fd, void* buf, int32_t nbytes) {
    long flags;
    int32_t bytes_to_read;

    if (buf == NULL) return -1;

    term_struct_t* term_data = &(terms[get_current_group()]);

    while(1) {
        cli_and_save(flags);
        if (!(term_data->read_in_progress)) {
            term_data->read_in_progress = 1;
            sti();
            restore_flags(flags);
            break;
        }
        sti();
        restore_flags(flags);
    }

    /* Waits for a newline character */
    term_data->newline_seen = 0;
    while(1) {
        cli_and_save(flags);
        if (term_data->newline_seen) {
            sti();
            restore_flags(flags);
            break;
        }
        sti();
        restore_flags(flags);
    }

    cli_and_save(flags);

    /* Read min of nbytes and term_buff_size to buf */
    bytes_to_read = nbytes < term_data->term_buff_size ? nbytes : term_data->term_buff_size;
    memcpy(buf, term_data->term_buff, bytes_to_read);
    term_data->term_buff_size = 0; /* Clear buffer after reading */

    sti();
    restore_flags(flags);

    term_data->read_in_progress = 0;
    return bytes_to_read;
}

/* term_write 
 *  DESCRIPTION: Attempts to write nbytes from char* buf into terminal
 *       INPUTS: fd - Not used
 *               buf - char * of printable ascii characters to write
 *               nbytes - number of chars from buf to write
 *      OUTPUTS: None
 * RETURN VALUE: Number of bytes actually written
 * SIDE EFFECTS: Writes all possible bytes into terminal, stops writing when 
 *               buffer overflows
 */
int32_t term_write(int32_t fd, const void* buf, int32_t nbytes) {
    uint8_t * buff = (uint8_t *) buf;
    int i = 0;
    long flags;

    if (buf == NULL) return -1;

    int current_group = get_current_group();

    cli_and_save(flags);

    if (visible_group == current_group)
    {
        /* Print each char in buf to active group */
        for(i = 0; i < nbytes; i++) {
            __print_char(buff[i]);
        }
    }
    else
    {
        /* Save current video address and cursor position */
        char* saved_video_mem;
        int saved_x;
        int saved_y;
        get_video_mem(&saved_video_mem);
        get_cursor(&saved_x, &saved_y);
        /* Set video address and cursor position to active group */
        set_video_mem((char *)get_video_save_page(current_group));
        set_cursor(terms[current_group].cursor_x, terms[current_group].cursor_y);

        /* Print each char in buf to active group */
        for(i = 0; i < nbytes; i++) {
            __print_char(buff[i]);
        }

        /* Save active group's cursor position */
        get_cursor(&(terms[current_group].cursor_x), &(terms[current_group].cursor_y));

        /* Restore video address and cursor position */
        set_video_mem(saved_video_mem);
        set_cursor(saved_x, saved_y);
    }

    sti();
    restore_flags(flags);

    return i;
}

/* term_open
 *  DESCRIPTION: Opens either stdin or stdout given either of those filenames
 *               by getting the next available fd in the current PCB and
 *               populating that entry.
 *       INPUTS: filename - either "stdin" or "stdout"
 *      OUTPUTS: None
 * RETURN VALUE: Returns fd index number or FAILURE if invalid filename or no
 *               open fd.
 * SIDE EFFECTS: Marks one fd as in use
 */
int32_t term_open(const uint8_t* filename)
{
    file_t* fd_array;
    int fd;
    
    /* Open stdin */
    if (strncmp((int8_t*)filename, (const int8_t*)"stdin", strlen((int8_t*)filename)) == SUCCESS)
    {
        fd = get_new_fd();
        if (fd == FAILURE)
            return FAILURE;
        
        /* Populate file descriptor index */
        fd_array = get_current_pcb()->fd_table;
        fd_array[fd].file_ops = &stdin_op_table;
        fd_array[fd].inode = 0;
        fd_array[fd].file_position = INIT_FILE_POS;
        fd_array[fd].flags = IN_USE;

        return fd;
    }
    /* Open stdout */
    else if (strncmp((int8_t*)filename, (const int8_t*)"stdout", strlen((int8_t*)filename)) == SUCCESS)
    {
        fd = get_new_fd();
        if (fd == FAILURE)
            return FAILURE;
        
        /* Populate file descriptor index */
        fd_array = get_current_pcb()->fd_table;
        fd_array[fd].file_ops = &stdout_op_table;
        fd_array[fd].inode = 0;
        fd_array[fd].file_position = INIT_FILE_POS;
        fd_array[fd].flags = IN_USE;

        return fd;
    }
    else
    {
        return FAILURE;
    }
}

/*
 * term_close 
 *  DESCRIPTION: Closes the terminal resource stdin or stdout by marking the
 *               given file descriptor as not in use.
 *       INPUTS: fd - file descriptor index of file
 *      OUTPUTS: None
 * RETURN VALUE: SUCCESS/FAILURE
 * SIDE EFFECTS: Clears file descriptor
 */
int32_t term_close(int32_t fd)
{
    file_t* fd_array = get_current_pcb()->fd_table;
    
    /* Check for valid fd */
    if (fd < 0 || fd >= FD_ARRAY_SIZE || fd_array[fd].flags == NOT_IN_USE)
        return FAILURE;

    /* Mark file descriptor as not in use */
    fd_array[fd].file_ops = NULL;
    fd_array[fd].flags = NOT_IN_USE;
    return SUCCESS;
}

/* __clear_term 
 *  DESCRIPTION: Clears the terminal buffers and the screen
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: See desscription
 */
void clear_term() {
    clear();
    long flags;

    cli_and_save(flags);
    restore_flags(flags);
}

int8_t add_char_term(uint8_t c) {
    if (c == '\b') {
        __backspace_term(); 
        return 0;
    }

    return __add_char_to_term(c);
}


/* __add_char_to_term 
 *  DESCRIPTION: Adds char c to terminal buffer if possible
 *       INPUTS: c - a char to add to the term buffer
 *      OUTPUTS: None
 * RETURN VALUE: 0 - success, non-zero else 
 * SIDE EFFECTS: Adds char c to buffer if possible, prints character to screen
 *               if buffer overflown or newline \n seen, switches working buffer
 *               and line buffer
 */
int8_t __add_char_to_term(uint8_t c) {
    long flags;
    cli_and_save(flags);

    term_struct_t* visible_term = &terms[visible_group];

    if (visible_term->term_buff_size < TERM_BUFFER_SIZE) {
        if (visible_term->term_buff_size == TERM_BUFFER_SIZE - 1 && c != '\n') {
            sti();
            restore_flags(flags);
            return -1;
        }

        if (c == '\t')
        {
            /* Treat tab as number of spaces until next index divisible by TAB_SIZE */
            int i;
            for (i = 0; i < TAB_SIZE; ++i)
            {
                visible_term->term_buff[visible_term->term_buff_size] = ' ';
                ++visible_term->term_buff_size;
                __print_char(' ');
                /* Stop adding spaces if buffer is full or reached TAB_SIZE */
                if (visible_term->term_buff_size == TERM_BUFFER_SIZE - 1 || visible_term->term_buff_size % TAB_SIZE == 0)
                    break;
            }
        }
        else
        {
            /* Add char to term buff and print to terminal */
            visible_term->term_buff[visible_term->term_buff_size] = c;
            visible_term->term_buff_size++;
            __print_char(c);
        }

        if (c == '\n') {
            /* clear term buff in preparation for new input */
            visible_term->newline_seen = 1;
            if (!visible_term->read_in_progress) {
                visible_term->term_buff_size = 0;
            }
        }

        sti();
        restore_flags(flags);
        return 0;
    }

    sti();
    restore_flags(flags);
    return -1;
}

/* __backspace_term 
 *  DESCRIPTION: Handles Backspace presses from keyboard
 *       INPUTS: None
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Performs destructive backspace by moving cursor back and overwriting video mem.
 *               If term_buff_size == 0, does nothing
 */
void __backspace_term() {
    long flags;
    cli_and_save(flags);

    if (terms[visible_group].term_buff_size > 0) {
        terms[visible_group].term_buff_size--;      /* Remove last character from term buff */
        __print_char('\b');                         /* Perform destructive backspace on other characters */
    } 

    sti();
    restore_flags(flags);
}

/* __print_char 
 *  DESCRIPTION: Renders character c on screen without modifying the terminal buffer
 *       INPUTS: c - the character to print
 *      OUTPUTS: None
 * RETURN VALUE: None
 * SIDE EFFECTS: Prints character to screen, 4 spaces for tab, desctructive backspace for \b
 */
void __print_char(char c) {
    if (c == '\b') {
        printf("\b \b"); 
    } else {
        printf("%c", c);
    }
}

/* switch_term
 *  DESCRIPTION: Updates the visible terminal to group_num
 *       INPUTS: group_num - terminal number betweeen 0 and MAX_PROCESS_GROUPS
 *      OUTPUTS: N/A
 * RETURN VALUE: FAILURE or New Visible Term Number
 * SIDE EFFECTS: Saves old visible screen to save page and copies save page of new term
 *                  to visible screen.
 */
int32_t switch_term(int32_t group_num) {
    long flags;

    cli_and_save(flags);

    /* Sanitize input */
    if (group_num < 0 || group_num >= MAX_PROCESS_GROUPS) {
        return FAILURE;
    }

    /* Save cursor coordinates */
    get_cursor(&terms[visible_group].cursor_x, &terms[visible_group].cursor_y);

    /* Save current group's video: copy kernel video to group's video page */
    memcpy((void*)get_video_save_page(visible_group), (void*)VIDEO_KERNEL, PAGE_SIZE);

    /* Switch visible_group to process_group_num */
    visible_group = group_num;

    /* Restore cursor coordinates */
    set_cursor(terms[visible_group].cursor_x, terms[visible_group].cursor_y);
    
    /* Restore next group's video: copy group's video to kernel video page */
    memcpy((void*)VIDEO_KERNEL, (void*)get_video_save_page(visible_group), PAGE_SIZE);

    sti();
    restore_flags(flags);

    return visible_group;
}

/* get_video_save_page
 *  DESCRIPTION: Returns virtual address of save page for group_num
 *       INPUTS: group_num - 0 to MAX_PROCESS_GROUPS
 *      OUTPUTS: N/A
 * RETURN VALUE: Address of video save page for group_num
 * SIDE EFFECTS: N/A
 */
uint32_t get_video_save_page(int32_t group_num)
{
    /* Ensure that input is a valid group number */
    if (group_num < 0 || group_num > MAX_PROCESS_GROUPS)
        return FAILURE;

    /* Calculate virtual address of corresponding group */
    return VIDEO_GROUP_1 + group_num * PAGE_SIZE;
}
