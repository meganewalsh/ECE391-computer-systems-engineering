#ifndef FILE_SYS_H_
#define FILE_SYS_H_

#include "types.h"
#include "file.h"

#define FILENAME_LEN        32
#define DENTRY_RESERVED     24
#define BOOT_BLOCK_RESERVED 52
#define NUM_DIR_ENTRIES     63
#define NUM_DNODE_PER_INODE 1023
#define BLOCK_SIZE          4096

#define RTC_TYPE            0
#define DIR_TYPE            1
#define FILE_TYPE           2


file_op_table_t file_type_op_table;
file_op_table_t dir_type_op_table;

typedef struct dentry
{
    char    filename[FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t  reserved[DENTRY_RESERVED];
} dentry_t;

typedef struct boot_block
{
    int32_t dir_count;
    int32_t inode_count;
    int32_t data_count;
    int8_t  reserved[BOOT_BLOCK_RESERVED];
    dentry_t dir_entries[NUM_DIR_ENTRIES];
} boot_block_t;

typedef struct inode
{
    int32_t length;
    int32_t data_block_num[NUM_DNODE_PER_INODE];
} inode_t;

extern file_op_table_t file_type_op_table;
extern file_op_table_t dir_type_op_table;
extern file_op_table_t stdin_op_table;
extern file_op_table_t stdout_op_table;


void file_sys_init(void* file_sys_img);

/* File Open/Close/Read/Write functions */
extern int32_t file_open(const uint8_t* filename);
extern int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t file_close(int32_t fd);

/* Directory Open/Close/Read/Write functions */
extern int32_t dir_open(const uint8_t* filename);
extern int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t dir_close(int32_t fd);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_data(uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t length);

#endif
