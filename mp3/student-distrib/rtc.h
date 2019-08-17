#ifndef RTC_H_
#define RTC_H_

#include "types.h"
#include "tests.h"
#include "file.h"

#define IRQ_8           8
#define RTC_PORT0       0x70
#define RTC_PORT1       0x71
#define RTC_RATE        0x0A
#define RTC_RATE_2Hz	14
#define RTC_REG_A_NMI   0x8A
#define RTC_REG_B_NMI   0x8B
#define RTC_REG_C       0x0C
#define MAX_RTC_RATE    32768
#define MIN_RTC_RATE    2


#if RUN_TESTS
volatile int rtc_count;
volatile int tests_rtc_read_waited_for_int;
volatile int32_t tests_rtc_curr_hz;
#endif

file_op_table_t rtc_type_op_table;

void rtc_init();
void rtc_wrapper(void);
int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);

#endif
