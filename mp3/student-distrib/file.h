#ifndef FILE_H_
#define FILE_H_

#include "lib.h"

#define FD_ARRAY_SIZE       8
#define NOT_IN_USE          0
#define IN_USE              1
#define INIT_FILE_POS       0

typedef struct file_operations_table
{
    int32_t (*read)(int32_t, void*, int32_t);
    int32_t (*write)(int32_t, const void*, int32_t);
    int32_t (*open)(const uint8_t*);
    int32_t (*close)(int32_t);
} file_op_table_t;

/* File Descriptor Entry */
typedef struct file_descriptor_entry
{
    file_op_table_t* file_ops;
    uint32_t inode;
    uint32_t file_position;
    uint32_t flags;
} file_t;


#endif
