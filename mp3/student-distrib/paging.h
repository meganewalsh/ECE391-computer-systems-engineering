#ifndef PAGING_H_
#define PAGING_H_


#define NUM_PAGE_ENTRIES    1024        /* Number of entries in PD and PTs */
#define PAGE_SIZE           4096        /* Page size is 4KiB */
#define PD_VIDEO_ENTRY      0           /* Index of video memory in the PD */
#define PT_VIDEO_ENTRY      184         /* Index of video memory in PT0 */
#define PD_KERNEL           1           /* Index of kernel in the PD */

#define PDE_PAGE_SIZE       0x80        /* Bit 7 of PDE is page size */
#define PDE_USER_SUPERVISOR 0x4         /* Bit 2 of PDE is user/supervisor */
#define PDE_READ_WRITE      0x2         /* Bit 1 of PDE is read/write */
#define PDE_PRESENT         0x1         /* Bit 0 of PDE is present */

#define VIDEO_KERNEL        0xB8000     /* Location of video memory */
#define VIDEO_USER          0xB9000     /* Virtual address of user's page to video memory */
#define VIDEO_GROUP_1       0xBA000     /* Address of process group 1's saved video page */
#define VIDEO_GROUP_2       0xBB000     /* Address of process group 2's saved video page */
#define VIDEO_GROUP_3       0xBC000     /* Address of process group 3's saved video page */
#define KERNEL_LOC          0x400000    /* Location of kernel in physical memory */
#define KERNEL_LOC_END      0x800000    /* First location after end of kernel memory */
#define PROG_PAGE_SIZE      0x400000    /* Each page in prog mem is 4MB in size */

#define ALIGN_MB            0xFFC00000  /* Masks bits to align address to 4 MB */
#define FLAG_MASK           0xFFFFF000  /* Masks flag bits of PD and PT entries */
#define BITS_TO_PD_IDX      22          /* Number of bits to shift PDE to get PD index */
#define BITS_TO_PT_IDX      12          /* Number of bits to shift PTE to get PT index (must mask out upper bits still) */
#define PT_MASK             0x03FF      /* Masks out all but 10 lowest bits */

#define TRUE                1           /* To be used for map page flags */
#define FALSE               0

void paging_init();

/* Maps page of virtual mem to physical location provided */
void map_page(uint32_t virtual_loc, uint32_t phys_loc, uint8_t read_write, uint8_t user, uint8_t page_size);

/* Unmaps page of virtual mem */
void unmap_page(uint32_t virtual_loc, uint8_t page_size);

#endif
