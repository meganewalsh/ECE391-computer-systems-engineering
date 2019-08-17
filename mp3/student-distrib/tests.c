#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "rtc.h"
#include "term.h"
#include "device/keyboard.h"
#include "file_sys.h"
#include "file.h"
#include "system.h"
#include "pcb.h"


#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");
#define TEST_FINISHED \
    printf("[TEST %s] Finished %s:%d\n", __FUNCTION__, __FILE__, __LINE__)


static inline void assertion_failure(){
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

#if RUN_TESTS

/* Checkpoint 1 tests */

/* IDT Test - Example
 *   DESCRIPTION: Asserts that first 10 IDT entries are not NULL
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: None
 *      COVERAGE: Load IDT, IDT definition
 *         FILES: x86_desc.h/S
 */
int idt_test(){
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 20; ++i){
        if ((idt[i].offset_15_00 == NULL) && 
                (idt[i].offset_31_16 == NULL)){
            printf("before assert fail");
            assertion_failure();
            printf("assert fail");
            result = FAIL;
        }
    }

    return result;
}

/* Paging Tests */
/*
 * Paging Tests 0-4
 *   DESCRIPTION: Test 0: Tries to read value at address 0.
 *        Test 1: Tries to read value before video memory.
 *        Test 2: Tries to read value after video memory.
 *        Test 3: Tries to read value before kernel memory.
 *        Test 4: Tries to read value after kernel memory.
 *        INPUTS: none
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: Expects page fault exception
 *      COVERAGE: init_paging
 *         FILES: paging.c/.h
 */
int paging_test_0()
{
    TEST_HEADER;

    /* Access at address 0 should throw exception */
    int* ptr = (int*)0;
    int value = *ptr;
    ++value;

    /* If doesn't throw exception, test fails */
    return FAIL;
}

int paging_test_1()
{
    TEST_HEADER;

    /* Access at int before 0xB8000 (0xB8000 - 4) should throw exception */
    int* ptr = (int*)(VIDEO_KERNEL - 4);
    int value = *ptr;
    ++value;

    /* If doesn't throw exception, test fails */
    return FAIL;
}

int paging_test_2()
{
    TEST_HEADER;

    /* Access at 0xB9000 should throw exception */
    int* ptr = (int*)(VIDEO_USER);
    int value = *ptr;
    ++value;

    /* If doesn't throw exception, test fails */
    return FAIL;
}

int paging_test_3()
{
    TEST_HEADER;

    /* Access at int before 0x400000 (0x400000 - 4) should throw exception */
    int* ptr = (int*)(KERNEL_LOC - 4);
    int value = *ptr;
    ++value;

    /* If doesn't throw exception, test fails */
    return FAIL;
}

int paging_test_4()
{
    TEST_HEADER;

    /* Access at 0x800000 should throw exception */
    int* ptr = (int*)(KERNEL_LOC_END);
    int value = *ptr;
    ++value;

    /* If doesn't throw exception, test fails */
    return FAIL;
}

/*
 * Paging Tests for Video and Kernel Memory
 *   DESCRIPTION: Reads every value within video and kernel memory pages.
 *        INPUTS: none
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: none
 *      COVERAGE: init_paging
 *         FILES: paging.c/.h
 */
int paging_test_video_and_kernel()
{
    int* addr;
    int value;

    /* Read from video memory */
    for (addr = (int*)VIDEO_KERNEL; addr < (int*)VIDEO_USER; addr += 4)
    {
        value = *addr;
    }

    /* Read from kernel memory */
    for (addr = (int*)KERNEL_LOC; addr < (int*)KERNEL_LOC_END; addr += 4)
    {
        value = *addr;
    }

    return PASS;
}

/* Checkpoint 2 tests */

/* Variables used for rtc_test */
int32_t Hz_FAST = 32;
int32_t Hz_SLOW = 2;
static int32_t Hz_Curr = 2;

/*
 * test_rtc_open_close
 *   DESCRIPTION: Test open/close rtc functionality
 *        INPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 *      COVERAGE: rtc_open(), rtc_close()
 *         FILES: rtc.c/.h
 */
int test_rtc_open_close() {
    int32_t fd = 0;
    uint8_t filename[] = "rtc"; /* TODO change */
    int ret = PASS;

    TEST_HEADER;
    fd = rtc_open(filename);

    if (fd == -1 ) ret = FAIL;

    if (rtc_close(fd)) ret = FAIL;

    return ret;
}

/* TODO update when virtualized rtc is implemented
 * test_rtc_read()
 *   DESCRIPTION: Tests whether read() returns on interrupt
 *        INPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 *      COVERAGE: rtc_read()
 *         FILES: rtc.c/.h
 */
int test_rtc_read() {
    int32_t fd;
    int ret = PASS;

    TEST_HEADER;

    fd = rtc_open((const uint8_t*)"rtc");

    tests_rtc_read_waited_for_int = 0;

    rtc_read(fd, NULL, 0);

    if (!tests_rtc_read_waited_for_int) {
        ret = FAIL;
    }

    rtc_close(fd);

    return ret;
}

/*
 * test_rtc_write
 *   DESCRIPTION: Periodically switches RTC between two frequencies. Called
 *                from rtc_wrapper in rtc.c.
 *        INPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 *      COVERAGE: rtc_write
 *         FILES: rtc.c/.h
 */
int test_rtc_write() {
    int32_t fd;
    int ret = PASS;
    int sw = 0;
    int offset = -1;

    TEST_HEADER;

    fd = rtc_open((const uint8_t*)"rtc");

    Hz_Curr = Hz_SLOW;
    rtc_write(fd, &Hz_Curr, 4);

    if (Hz_Curr != tests_rtc_curr_hz) {
        ret = FAIL;
    }

    printf(" ");
    
    while(sw < 10) {
        /* toggle between fast and slow every 10 cycles for 10 switches */
        if (rtc_count % 10 == 0) {
            sw++;
            printf("%c", '0' + sw); /* Move forward one */
            if (Hz_Curr == Hz_FAST)
                Hz_Curr = Hz_SLOW;
            else
                Hz_Curr = Hz_FAST;
            rtc_write(fd, &Hz_Curr, 4);
            if (Hz_Curr != tests_rtc_curr_hz) {
                ret = FAIL;
            }
        }
        rtc_read(fd, NULL, 0);

        /* Move cursor back so next write overwrites current */
        printf("\b%c", ('0' + sw) + offset);    /* print sw number toggling back and forth between next num */
        offset = -offset;
    }

    printf("\n");

    rtc_close(fd);

    return ret;
}

/*
 * test_rtc_invalid
 *   DESCRIPTION: Tries to set rtc rate to invalid value. Rate should not
 *                change from 2Hz
 *        INPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 *      COVERAGE: rtc_write
 *         FILES: rtc.c/.h
 */
int test_rtc_invalid() {
    int32_t fd;
    int ret = PASS;

    TEST_HEADER;

    rtc_open((const uint8_t*)"rtc");

    /* Note: this frequency is not a power of 2 */
    Hz_Curr = 32700; /* In Hz */
    rtc_write(0, &Hz_Curr, 0);

    if (tests_rtc_curr_hz == Hz_Curr) {
        ret = FAIL; 
    }

    rtc_close(fd);

    return ret;
}


/* Terminal Driver Tests */

/* NOTE: Does not work after CP3
 * term_open_close_test
 *   DESCRIPTION: Tests terminal open/close operations
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: None
 *      COVERAGE: term_open(), term_close()
 *         FILES: device/keyboard.c/h
 */
int term_open_close_test() {
    int ret = PASS;
    uint8_t filename[] = "file";
    int32_t fd;

    TEST_HEADER;

    fd = term_open(filename);

    if (fd) ret = FAIL;
    if(term_close(fd)) ret = FAIL;

    return ret;
}

/* NOTE: Does not work after CP3
 * term_read_write_test
 *   DESCRIPTION: Tests terminal read/write operations
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: prints some characters to screen, writes to terminal
 *      COVERAGE: term_write(), term_read()
 *         FILES: device/keyboard.c/h
 */
int term_read_write_test() {
    int ret = PASS;
    uint8_t filename[] = "term";
    char buf[TTBUF_SIZE]= {'I','f','y','o','u','s','e','e','m','e','t','h','e','n','p', 'a','s','s'};
    char rbuf[TTBUF_SIZE];
    int32_t fd;

    TEST_HEADER;

    fd = term_open(filename);

    /* Test to see if newline can be read */
    printf("\nPlease press enter and only enter...\n");
    if(term_read(fd, rbuf, TTBUF_SIZE - 1) != 1) ret = FAIL; 
    if (rbuf[0] != '\n') ret = FAIL; 

    /* Test to see if 1 can be read */
    printf("\nPlease press 1 then enter and only 1 then enter...\n");
    if(term_read(fd, rbuf, TTBUF_SIZE - 1) != 2) ret = FAIL; 
    if (rbuf[0] != '1') ret = FAIL; 
    if (rbuf[1] != '\n') ret = FAIL; 

    /* Write buf to terminal */
    if (term_write(fd, buf, TTBUF_SIZE) != TTBUF_SIZE) {
        /* Fail if whole buff is not written */
        ret = FAIL;
    }

    printf("\nPlease press 1 if success, 0 if not (then enter). Success if this line is preceded by a special message \n");

    /* Read what was written to terminal */
    if (term_read(fd, rbuf, TTBUF_SIZE - 1) != 2) {    /* Newline char should be added hence size difference */
        /* Fail if not everything is read back */
        ret = FAIL;
    }
    if (rbuf[0] != '1') ret = FAIL;
    if (rbuf[1] != '\n') ret = FAIL;

    term_close(fd);

    printf("\n");

    return ret;
}

/* NOTE: Does not work after CP3
 * term_buff_overflow_test
 *   DESCRIPTION: Tests terminal buffer overflow functionality
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: prints some characters to screen, writes to terminal
 *      COVERAGE: term_write(), term_read()
 *         FILES: device/keyboard.c/h
 */
int term_buff_overflow_test() {
    int ret = PASS;
    uint8_t filename[] = "file";
    char rbuf[TERM_BUFFER_SIZE + 1];
    int32_t fd;

    TEST_HEADER;

    rbuf[TERM_BUFFER_SIZE] = 0xFF;

    fd = term_open(filename);

    printf("\nPlease attempt to overflow the buffer by inputing more than %d chars\n", 
                    TERM_BUFFER_SIZE);
    printf("If more than %d chars print please fail. Otherwise, try using backspace to edit", 
                    TERM_BUFFER_SIZE - 1);
    printf("If backspace appears to work, set the last value displayed to 'P' and press enter, fail by leaving the last character as anything else\n", 
                    TERM_BUFFER_SIZE - 1);

    /* Read back what was written */
    if (term_read(fd, rbuf, TERM_BUFFER_SIZE) != TERM_BUFFER_SIZE) {
        /* FAIL if the size is not equal to the max line size (user didn't max out buffer) */
        ret = FAIL;
    }
    if (rbuf[TERM_BUFFER_SIZE - 2] != 'P') ret = FAIL;

    printf("\n Is the following line what you inputted? P-enter for yes, F-enter for no\n");
    term_write(fd, rbuf, TERM_BUFFER_SIZE);
    printf("\n");

    if (term_read(fd, rbuf, TERM_BUFFER_SIZE) != 2) ret = FAIL;
    if (rbuf[0] != 'P') ret = FAIL;
    if (rbuf[1] != '\n') ret = FAIL;

    term_close(fd);

    printf("\n");

    return ret;
}

/* File system tests */

/*
 * test_file_open_close
 *   DESCRIPTION: Opens all possible file descriptors, tries to open more than
 *                8 file descriptors, closes all descriptors, and tests that a
 *                descriptor can once more be opened and that file_open does
 *                not succeed for non-existent files or names >32 characters.
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: none
 *      COVERAGE: file_open(), file_close()
 *         FILES: file_sys.c/h, file.h
 */
int test_file_open_close()
{
    uint8_t filename[] = "hello";
    uint8_t filename_not_exist[] = "test.txt";
    uint8_t filename_long[] = "verylargetextwithverylongname.txt";
    int i;

    /* Open all possible files */
    for (i = 0; i < FD_ARRAY_SIZE; ++i)
    {
        if (file_open(filename) == FAILURE)
            return FAIL;
    }

    /* Test that only 8 fd are assigned */
    if (file_open(filename) != FAILURE)
        return FAIL;

    /* fd 0 and 1 should be unclosable */
    if (file_close(0) == SUCCESS || file_close(1) == SUCCESS)
        return FAIL;

    /* Close all other files */
    for (i = 2; i < FD_ARRAY_SIZE; ++i)
    {
        file_close(i);
    }

    /* Check reusability of FDs */
    int32_t fd = file_open(filename);
    if (fd == FAILURE)
        return FAIL;
    file_close(fd);

    /* Test non-existent filename */
    fd = file_open(filename_not_exist);
    if (fd != FAILURE)
        return FAIL;
    file_close(fd);

    /* Test filename >32 characters */
    fd = file_open(filename_long);
    if (fd != FAILURE)
        return FAIL;
    file_close(fd);

    return PASS;
}

/*
 * test_file_write_and_dir
 *   DESCRIPTION: Tests that file_write and dir_write don't do anything.
 *        INPUTS: None
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: none
 *      COVERAGE: file_write(), dir_write()
 *         FILES: file_sys.c/h, file.h
 */
int test_file_write_and_dir()
{
    uint8_t filename[] = "hello";
    uint8_t dir_name[] = ".";
    uint8_t buf[BUF_SIZE];

    /* Test write file */
    int32_t file_fd = file_open(filename);
    if (file_fd != FAILURE && file_write(file_fd, buf, BUF_SIZE) != FAILURE)
    {
        file_close(file_fd);
        return FAIL;
    }
    
    /* Test write dir */
    int32_t dir_fd = file_open(dir_name);
    if (dir_fd != FAILURE && dir_write(dir_fd, buf, BUF_SIZE) != FAILURE)
    {
        dir_close(dir_fd);
        return FAIL;
    }

    return PASS;
}

/*
 * test_dir_read
 *   DESCRIPTION: Emulates ls by continuously calling dir_read until no more
 *                directory entries are read. Success must be verified visually
 *        INPUTS: none
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: Writes all filenames in directory to terminal
 *      COVERAGE: dir_read()
 *         FILES: file_sys.c/h, file.h
 */
int test_dir_read()
{
    uint8_t term_name[] = "file";
    uint8_t dir_name[] = ".";
    uint8_t buf[BUF_SIZE + 1];

    int32_t term_fd = term_open(term_name);

    int32_t fd = dir_open(dir_name);
    int32_t ret = dir_read(fd, buf, BUF_SIZE);
    while (ret > 0)
    {
        buf[BUF_SIZE] = '\n';
        term_write(term_fd, buf, BUF_SIZE + 1);
        ret = dir_read(fd, buf, BUF_SIZE);
    }
    dir_close(fd);
    term_close(term_fd);

    if (ret == FAILURE)
        return FAIL;
    else
        return PASS;
}

/*
 * test_file_read
 *   DESCRIPTION: Writes contents of given file to terminal in BUF_SIZE chunks.
 *                Success must be verified visually.
 *        INPUTS: filename - char string of filename in file system to print
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: Writes file to terminal
 *      COVERAGE: file_read()
 *         FILES: file_sys.c/h, file.h
 */
int test_file_read(uint8_t* filename)
{
    uint8_t term_name[] = "";
    uint8_t buf[BUF_SIZE];

    int32_t term_fd = term_open(term_name);

    int32_t fd = file_open(filename);
    int32_t bytes_read = file_read(fd, buf, BUF_SIZE);
    while (bytes_read > 0)
    {
        term_write(term_fd, buf, bytes_read);
        bytes_read = file_read(fd, buf, BUF_SIZE);
        break;  /* only print first BUF_SIZE chars */
    }
    file_close(fd);
    term_close(term_fd);
    
    if (bytes_read == FAILURE)
        return FAIL;
    else
        return PASS;
}

/*
 * test_read_all_files
 *   DESCRIPTION: Each file in the directory (besides "." and "rtc") is printed
 *                to the terminal after prompting and waiting for ENTER.
 *                Success must be verified visually.
 *        INPUTS: none
 *  RETURN VALUE: PASS/FAIL
 *  SIDE EFFECTS: Writes file data to terminal
 *      COVERAGE: dir_read()
 *         FILES: file_sys.c/h, file.h
 */
int test_read_all_files()
{
    uint8_t term_name[] = "";
    uint8_t dir_name[] = ".";
    uint8_t rtc_name[] = "rtc";
    uint8_t message[] = "\nPress ENTER to print next file: \0";
    uint8_t read_buf[TERM_BUFFER_SIZE];
    uint8_t filename[BUF_SIZE + 1];     /* Include room for \0 sentinel */
    filename[BUF_SIZE] = '\0';

    int32_t term_fd = term_open(term_name);

    /* For each file in the directory */
    int32_t dir_fd = dir_open(dir_name);
    int32_t ret = dir_read(dir_fd, filename, BUF_SIZE);
    while (ret > 0)
    {
        /* Don't attempt to read "." (length 1) or "rtc" (length 3) */
        if (strncmp((int8_t*)filename, (int8_t*)dir_name, 1) == 0 ||
            strncmp((int8_t*)filename, (int8_t*)rtc_name, 3) == 0)
        {
            /* Get next file */
            ret = dir_read(dir_fd, filename, BUF_SIZE);
            continue;
        }

        /* Write message and wait for ENTER */
        term_write(term_fd, message, strlen((int8_t*)message));
        term_write(term_fd, filename, BUF_SIZE);
        term_read(term_fd, read_buf, 0);

        /* Read file and write to terminal */
        int32_t file_fd = file_open(filename);
        int32_t bytes_read = file_read(file_fd, read_buf, TERM_BUFFER_SIZE);
        while (bytes_read > 0)
        {
            term_write(term_fd, read_buf, bytes_read);
            bytes_read = file_read(file_fd, read_buf, TERM_BUFFER_SIZE);
        }
        file_close(file_fd);
        
        if (bytes_read == FAILURE)
            return FAIL;

        /* Get next file */
        ret = dir_read(dir_fd, filename, BUF_SIZE);
    }
    dir_close(dir_fd);
    term_close(term_fd);

    if (ret == FAILURE)
        return FAIL;
    else
        return PASS;
}


/* Checkpoint 3 tests */

/* Opens 2 too many FDs to make sure open returns FAILURE. Closes all proper FDs */
int test_system_open_all_fds()
{
    int count = 0;
    int i;

    /* Open all FDs plus 2 (because 0 and 1 are already open by term) */
    for (i = 0; i < FD_ARRAY_SIZE; ++i)
    {
        if (FAILURE == system_open((uint8_t*)"."))
        {
            ++count;
        }
    }

    /* Close all FDs but 0 and 1 */
    for (i = 2; i < FD_ARRAY_SIZE; ++i)
    {
        system_close(i);
    }

    return count == 2 ? PASS : FAIL;
}

/* Checks that system_open catches invalid filenames */
int test_system_file_opennames()
{
    int count = 0;

    if (FAILURE != system_open((uint8_t*)"helloo"))
        ++count;
    if (FAILURE != system_open((uint8_t*)"shel"))
        ++count;
    if (FAILURE != system_open((uint8_t*)""))
        ++count;
    
    return count == 0 ? PASS : FAIL;
}

/* Checks that read, write, and close catch invalid/inactive FDs */
int test_system_invalid_fds()
{
    int count = 0;
    uint8_t buf[BUF_SIZE];
    int fd_neg = -1;
    int fd_big = 8;
    int fd_unopened = 6;

    if (FAILURE != system_read(fd_neg, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_write(fd_neg, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_close(fd_neg))
        ++count;
    
    if (FAILURE != system_read(fd_big, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_write(fd_big, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_close(fd_big))
        ++count;

    if (FAILURE != system_read(fd_unopened, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_write(fd_unopened, buf, BUF_SIZE-1))
        ++count;
    if (FAILURE != system_close(fd_unopened))
        ++count;
    
    return count == 0 ? PASS : FAIL;
}

/* Tests execution with leading/trailing spaces. Checks for success return codes */
int test_system_execute()
{
    int count = 0;
    uint8_t message1[] = "\n5 programs will now be executed.\n";
    uint8_t message2[] = "Please allow them to complete and halt.\n\n";

    term_write(1, (void*)message1, strlen((int8_t*)message1));
    term_write(1, (void*)message2, strlen((int8_t*)message2));

    if (SUCCESS != system_execute((uint8_t*)"shell"))
        ++count;
    if (SUCCESS != system_execute((uint8_t*)"                        ls                  "))
        ++count;
    if (SUCCESS != system_execute((uint8_t*)"syserr"))
        ++count;
    if (SUCCESS != system_execute((uint8_t*)"     hello     fake_arg   "))
        ++count;
    if (SUCCESS != system_execute((uint8_t*)"counter"))
        ++count;

    return count == 0 ? PASS : FAIL;
}



/* Checkpoint 4 tests */

/*
 *  TODO comment. Manually tested function, do not add in checkpoint tests
 */
int test_page_map_unmap() {
    uint32_t virt_addr = 0xF0000000;   /* Arbitrary numbers outside kernel space */
    uint32_t phys_addr = 0xF0000000;

    map_page(virt_addr, phys_addr, TRUE, TRUE, TRUE);   /* Map 4MB page */
    unmap_page(virt_addr, TRUE);                        /* Unmap 4MB page */

    virt_addr = 0x00001000;   /* Arbitrary numbers in first 4kB */
    phys_addr = 0x00000000;

    map_page(virt_addr, phys_addr, TRUE, TRUE, FALSE);       /* Map 4kB page inside first PT */
    unmap_page(virt_addr, FALSE);                            /* Unmap 4kB page inside first PT */
    
    return FAIL;
}

/* Checkpoint 5 tests */

/* Wrapper function which calls all tests relevant to checkpoint 1 */
void checkpoint1() {
    TEST_HEADER;
    /***** Checkpoint 1 Tests *****/

    TEST_OUTPUT("idt_test", idt_test());

    /* Test each of the 20 exception vectors */
    // asm ("int $0x00");
    // asm ("int $0x01");
    // asm ("int $0x02");
    // asm ("int $0x03");
    // asm ("int $0x04");
    // asm ("int $0x05");
    // asm ("int $0x06");
    // asm ("int $0x07");
    // asm ("int $0x08");
    // asm ("int $0x09");
    // asm ("int $0x0A");
    // asm ("int $0x0B");
    // asm ("int $0x0C");
    // asm ("int $0x0D");
    // asm ("int $0x0E");
    // asm ("int $0x0F");
    // asm ("int $0x10");
    // asm ("int $0x11");
    // asm ("int $0x12");
    // asm ("int $0x13");

    /* Paging tests 0-4 should throw exceptions */
    // TEST_OUTPUT("paging_test_0", paging_test_0());
    // TEST_OUTPUT("paging_test_1", paging_test_1());
    // TEST_OUTPUT("paging_test_2", paging_test_2());
    // TEST_OUTPUT("paging_test_3", paging_test_3());
    // TEST_OUTPUT("paging_test_4", paging_test_4());

    TEST_OUTPUT("paging_test_video_and_kernel", paging_test_video_and_kernel());
}

/* Wrapper function which calls all tests relevant to checkpoint 2 */
void checkpoint2() {
    TEST_HEADER;

    /* Terminal tests */
    TEST_OUTPUT("term_open_close_test", term_open_close_test());
    TEST_OUTPUT("term_read_write_test", term_read_write_test());
    TEST_OUTPUT("term_buff_overflow_test", term_buff_overflow_test());

    /* RTC Tests */
    TEST_OUTPUT("test_rtc_open_close", test_rtc_open_close());
    TEST_OUTPUT("test_rtc_read", test_rtc_read());
    TEST_OUTPUT("test_rtc_write", test_rtc_write());
    TEST_OUTPUT("test_rtc_invalid", test_rtc_invalid());

    /* File System Tests */
    TEST_OUTPUT("test_file_open_close", test_file_open_close());
    TEST_OUTPUT("test_file_write_and_dir", test_file_write_and_dir());
    TEST_OUTPUT("test_dir_read", test_dir_read());
    uint8_t filename[] = "fish";
    TEST_OUTPUT("test_file_read", test_file_read(filename));    /* Prints first 32 chars of fish */
    TEST_OUTPUT("test_read_all_files", test_read_all_files());

    TEST_FINISHED;
}

/* Wrapper function which calls all tests relevant to checkpoint 3 */
void checkpoint3() {
    TEST_HEADER;
    
    TEST_OUTPUT("Open too many FDs", test_system_open_all_fds());
    TEST_OUTPUT("Open invalid filenames", test_system_file_opennames());
    TEST_OUTPUT("Read/write/close invalid FDs", test_system_invalid_fds());
    TEST_OUTPUT("Execute programs with white space", test_system_execute());

    TEST_FINISHED;
}

/* Wrapper function which calls all tests relevant to checkpoint 4 */
void checkpoint4() {
    TEST_HEADER;
    //TEST_OUTPUT("Page Map/Unmap", test_page_map_unmap()); // This is a manual test, do not uncomment me 

    /* TODO move checkpoint 4 test calls here */
    TEST_FINISHED;
}

/* Wrapper function which calls all tests relevant to checkpoint 5 */
void checkpoint5() {
    TEST_HEADER;

    /* TODO move checkpoint 5 test calls here */
    TEST_FINISHED;
}

#endif /* RUN_TESTS */

/* Test suite entry point */
void launch_tests() {
    #if RUN_TESTS
        #if RUN_CHECKPOINT_1
            checkpoint1();
        #endif

        #if RUN_CHECKPOINT_2
            checkpoint2();
        #endif

        #if RUN_CHECKPOINT_3
            checkpoint3();
        #endif

        #if RUN_CHECKPOINT_4
            checkpoint4();
        #endif

        #if RUN_CHECKPOINT_5
            checkpoint5();
        #endif
    #endif /* RUN_TESTS */

    printf("Tests Completed\n\n");
}

