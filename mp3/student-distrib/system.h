#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "types.h"

#define HALT_CODE_EXC           256         /* Return value of halt when an exception stops the program */

#define PROG_VIRT_ADDR          0x08000000  /* Programs loaded to 128MB */
#define PROG_OFFSET             0x48000     /* Program images loaded into the page at this offset */
#define PROG_EIP_OFF            24          /* Offset into file at which EIP for program is stored. Ranges from 24-27 */
#define SIZE_VARS_SCHEDULING    24          /* Size of local vars in schedule_next */
#define CHILD_EBP_OFF           68          /* Offset of child's ebp */

#define EXEC_MAGIC_LEN          4
#define EXEC_MAGIC_STR          0x464C457F
#define MAX_NUM_ARGS            3

extern void system_call_handler(void);
extern uint32_t sys_call(uint32_t sys_call_number, uint32_t param1, uint32_t param2, uint32_t param3);
extern int32_t static_start_shell(int32_t pid);

int32_t system_halt(uint32_t status);
int32_t system_execute(const uint8_t* command);
int32_t system_read(int32_t fd, void* buf, int32_t nbytes);
int32_t system_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t system_open(const uint8_t* filename);
int32_t system_close(int32_t fd);
int32_t system_getargs(uint8_t* buf, int32_t nbytes);
int32_t system_vidmap(uint8_t** screen_start);
int32_t system_sethandler (int32_t signum, void* handler_address);
int32_t system_sigreturn(void);

/* Other helper functions */
uint32_t get_prog_phys_addr(int32_t pid);


#endif /* SYSTEM_H_ */
