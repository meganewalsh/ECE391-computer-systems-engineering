#include "file_sys.h"
#include "file.h"
#include "lib.h"
#include "rtc.h"
#include "paging.h"
#include "term.h"
#include "pcb.h"

static boot_block_t* boot_blk;

/*
 * file_sys_init
 *   DESCRIPTION: Gets file system address and initializes the file and
 *                directory operations tables
 *        INPUTS: file_sys_img - address of file system image
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: See description
 */
void file_sys_init(void* file_sys_img)
{
    boot_blk = (boot_block_t*)file_sys_img;

    /* Initialize file type file_op_table */
    file_type_op_table.read = file_read;
    file_type_op_table.write = file_write;
    file_type_op_table.open = file_open;
    file_type_op_table.close = file_close;

    /* Initialize dir type file_op_table */
    dir_type_op_table.read = dir_read;
    dir_type_op_table.write = dir_write;
    dir_type_op_table.open = dir_open;
    dir_type_op_table.close = dir_close;
}

/*** File Open/Close/Read/Write functions ***/

/*
 * file_open
 *   DESCRIPTION: Does nothing
 *        INPUTS: filename - char string of filename to open
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS
 *  SIDE EFFECTS: none
 */
int32_t file_open(const uint8_t* filename)
{
    return SUCCESS;
}

/*
 * file_read
 *   DESCRIPTION: Reads nbytes from file into buf and updates file position.
 *        INPUTS: fd - file descriptor index of file
 *                nbytes - number of bytes to read
 *       OUTPUTS: buf - buffer in which to place read data
 *  RETURN VALUE: Number of bytes read and placed in the buffer or FAILURE.
 *  SIDE EFFECTS: File position is updated
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    file_t* fd_array = get_current_pcb()->fd_table;

   /* Check for valid parameters */
    if (fd < 0 || fd >= FD_ARRAY_SIZE || fd_array[fd].flags == NOT_IN_USE || buf == NULL || nbytes < 0)
        return FAILURE;

    /* Read data from file */
    int32_t bytes_read = read_data(fd_array[fd].inode, fd_array[fd].file_position, (uint8_t*)buf, (uint32_t)nbytes);

    /* Update file cursor */
    if (bytes_read == FAILURE)
    {
        return FAILURE;
    }
    else
    {
        fd_array[fd].file_position += bytes_read;
        return bytes_read;
    }
    
}

/*
 * file_write
 *   DESCRIPTION: Does nothing.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: FAILURE
 *  SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return FAILURE;
}

/*
 * file_close
 *   DESCRIPTION: Marks given file descriptor as not in use.
 *        INPUTS: fd - file descriptor index of file
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS/FAILURE
 *  SIDE EFFECTS: Clears file descriptor
 */
int32_t file_close(int32_t fd)
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


/*** Directory Open/Close/Read/Write functions ***/

/*
 * dir_open
 *   DESCRIPTION: Does nothing
 *        INPUTS: filename - always "."
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS
 *  SIDE EFFECTS: none
 */
int32_t dir_open(const uint8_t* filename)
{
    return SUCCESS;
}

/*
 * dir_read
 *   DESCRIPTION: For the dentry indexed by the file_position, reads min of
 *                nbytes and FILENAME_LEN of the filename with 0 padding into
 *                buf and increments the file_position.
 *        INPUTS: fd - file descriptor index of directory file
 *                nbytes - number of bytes to read
 *       OUTPUTS: buf - buffer in which to place filename
 *  RETURN VALUE: Number of bytes read and placed in the buffer or FAILURE.
 *  SIDE EFFECTS: none
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes)
{
    /* Check for valid parameters */
    if (fd < 0 || fd >= FD_ARRAY_SIZE || buf == NULL || nbytes < 0)
        return FAILURE;

    /* Get min of nbytes and FILENAME_LEN as # bytes to copy to buf */
    uint32_t num_bytes = nbytes < FILENAME_LEN ? nbytes : FILENAME_LEN;

    /* Get dentry number */
    file_t* fd_array = get_current_pcb()->fd_table;
    uint32_t dentry_num = fd_array[fd].file_position;

    /* Check for end of dentry list */
    if (dentry_num >= boot_blk->dir_count)
        return 0;

    /* Write filename to buf with 0 padding */
    strncpy((int8_t*)buf, boot_blk->dir_entries[dentry_num].filename, num_bytes);

    /* Increment file_position */
    ++fd_array[fd].file_position;

    /* Return number of bytes written */
    return num_bytes;
}

/*
 * dir_write
 *   DESCRIPTION: Does nothing.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: FAILURE
 *  SIDE EFFECTS: none
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return file_write(fd, buf, nbytes);
}

/*
 * dir_close
 *   DESCRIPTION: Marks given file descriptor as not in use.
 *        INPUTS: fd - file descriptor index of directory file
 *       OUTPUTS: none
 *  RETURN VALUE: SUCCESS/FAILURE
 *  SIDE EFFECTS: Clears file descriptor
 */
int32_t dir_close(int32_t fd)
{
    return file_close(fd);
}


/*** Internal Functions ***/

/*
 * read_dentry_by_name
 *   DESCRIPTION: Searches dentries of the boot block to match given file name.
 *                If match is found the entry is copied to dentry.
 *        INPUTS: fname - char string to match against dentry names
 *       OUTPUTS: dentry - matched dentry copied to struct at this pointer
 *  RETURN VALUE: Success/failure
 *  SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
    uint32_t name_len = strlen((int8_t*)fname);

    /* Check for invalid fnames that are too long */
    if (name_len > FILENAME_LEN)
        return FAILURE;

    int i;
    for (i = 0; i < boot_blk->dir_count; ++i)
    {
        if (boot_blk->dir_entries[i].inode_num < boot_blk->inode_count)
        {
            /* Get length of d_entry filename */
            uint32_t entry_length = strlen((int8_t*)boot_blk->dir_entries[i].filename);
            entry_length = entry_length > FILENAME_LEN ? FILENAME_LEN : entry_length;
            
            if (strncmp((int8_t*)fname, (int8_t*)(boot_blk->dir_entries[i].filename), name_len) == 0 &&
                strncmp((int8_t*)fname, (int8_t*)(boot_blk->dir_entries[i].filename), entry_length) == 0)
            {
                /* Match found */
                *dentry = boot_blk->dir_entries[i];
                return SUCCESS;
            }
        }
    }

    /* Finished search without match */
    return FAILURE;
}

/*
 * read_data
 *   DESCRIPTION: Reads up to length bytes from position offset in the file of
 *                the given inode number. Returns the number of bytes read and
 *                placed in the buffer.
 *        INPUTS: inode - inode number to corresponding file to read
 *                offset - position in file data at which to start reading
 *                length - number of bytes to read
 *       OUTPUTS: buf - buffer into which read bytes are placed
 *  RETURN VALUE: Number of bytes read and placed in the buffer. An number less
 *                than length indicates that the EOF reached.
 *  SIDE EFFECTS: none
 */
int32_t read_data(uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t length)
{
    /* If length is 0, do nothing and return 0 */
    if (length == 0)
        return 0;

    /* Check for invalid inode number */
    if (inode_idx >= boot_blk->inode_count)
        return FAILURE;

    inode_t* inode = (inode_t*)((uint32_t)boot_blk + (inode_idx + 1) * BLOCK_SIZE);
    uint8_t* dnodes_start = (uint8_t*)((uint32_t)boot_blk + (boot_blk->inode_count + 1) * BLOCK_SIZE);

    /* Check that offset doesn't extend beyond length of file */
    if (offset >= inode->length)
        return 0;

    /* Get starting place within dnodes based on offset */
    int dnode_num = offset / BLOCK_SIZE;
    int pos_in_dnode = offset % BLOCK_SIZE;

    /* Get first dnode */
    int dnode_idx = inode->data_block_num[dnode_num];
    uint8_t* dnode = (uint8_t*)(dnodes_start + dnode_idx * BLOCK_SIZE);

    int buf_idx = 0;    /* Tracks number of bytes read to buf */

    /* Check for enough bytes read or end of file */
    while (buf_idx < length && offset + buf_idx < inode->length)
    {
        /* Check for end of dnode */
        if (pos_in_dnode >= BLOCK_SIZE)
        {
            /* Get next dnode (not end of file so must be more) */
            ++dnode_num;
            dnode_idx = inode->data_block_num[dnode_num];
            dnode = (uint8_t*)(dnodes_start + dnode_idx * BLOCK_SIZE);

            /* Reset position */
            pos_in_dnode = 0;
        }

        /* Read value into buf */
        buf[buf_idx] = *(dnode + pos_in_dnode);

        /* Prepare for next read */
        ++buf_idx;
        ++pos_in_dnode;
    }

    return buf_idx;
}

