/*
 * File: paging.c
 */
#include "lib.h"
#include "paging.h"

#define SPACE               0x07200720  /* ASCII value of a space */

/* Local Helpers */
void __flush_tlb();

/* Define page directory and page table aligned to 4KiB page */
unsigned int page_directory[NUM_PAGE_ENTRIES] __attribute__((aligned (PAGE_SIZE)));
unsigned int page_table_0[NUM_PAGE_ENTRIES] __attribute__((aligned (PAGE_SIZE)));

/* @sjw2
 * init_paging
 *   DESCRIPTION: Initializes page directory and single page table with video
 *                memory (4KiB page) and kernel (4MiB page). Sets appropriate
 *                control registers to enable paging.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Initializes PD and PT0. Modifies CR0, CR3, and CR4
 */
void paging_init()
{
    /* Enable page size extension (PSE); CR4 bit 4 */
    asm volatile(
        "movl   %%cr4, %%eax        \n\
         orl    $0x10, %%eax        \n\
         movl   %%eax, %%cr4        \n"
        : /* outputs */
        : /* inputs */
        : "eax", "cc", "memory" /* clobbers */
    );

    /* Initialize PD */
    memset(page_directory, 0, sizeof(uint32_t)*NUM_PAGE_ENTRIES);                                /* Clear all entries */
    page_directory[PD_VIDEO_ENTRY] = (unsigned int)page_table_0 & FLAG_MASK;                     /* Mask out flag bits of pointer */
    page_directory[PD_VIDEO_ENTRY] |= PDE_READ_WRITE | PDE_USER_SUPERVISOR | PDE_PRESENT;        /* Set appropriate flag bits */
    page_directory[PD_KERNEL] = (unsigned int)KERNEL_LOC & FLAG_MASK;                            /* Mask out flag bits of pointer */
    page_directory[PD_KERNEL] |= PDE_PAGE_SIZE | PDE_READ_WRITE | PDE_PRESENT;                   /* Set appropriate flag bits */

    /* Fill PT */
    memset(page_table_0, 0, sizeof(uint32_t)*NUM_PAGE_ENTRIES);                                  /* Clear all entries */
    page_table_0[PT_VIDEO_ENTRY] = (unsigned int)VIDEO_KERNEL & FLAG_MASK;                       /* Mask out flag bits of pointer */
    page_table_0[PT_VIDEO_ENTRY] |= PDE_READ_WRITE | PDE_PRESENT;                                /* Set appropriate flag bits */

    /* Map video save pages */
    map_page(VIDEO_GROUP_1, VIDEO_GROUP_1, TRUE, FALSE, FALSE);
    map_page(VIDEO_GROUP_2, VIDEO_GROUP_2, TRUE, FALSE, FALSE);
    map_page(VIDEO_GROUP_3, VIDEO_GROUP_3, TRUE, FALSE, FALSE);

    /* Clear video save pages */
    int* vid_group1_ptr = (void*)VIDEO_GROUP_1;
    int* vid_group2_ptr = (void*)VIDEO_GROUP_2;
    int* vid_group3_ptr = (void*)VIDEO_GROUP_3;

    int i;
    for (i = 0; i*sizeof(int) < PAGE_SIZE; i++){
        vid_group1_ptr[i] = SPACE;
        vid_group2_ptr[i] = SPACE;
        vid_group3_ptr[i] = SPACE;
    }

    /* Set upper 20 bits of CR3 to point to PD (PDBR) */
    unsigned int page_directory_base = (unsigned int)page_directory;
    asm volatile(
        "movl   %%eax, %%cr3"
        :
        : "a" (page_directory_base)
        : "cc", "memory"
    );

    /* Enable paging; set PG, CR0 bit 31 */
    asm volatile(
        "movl   %%cr0, %%eax            \n\
         orl    $0x80000001, %%eax      \n\
         movl   %%eax, %%cr0            \n"
        :
        :
        : "eax", "cc", "memory"
    );
}

/*
 * map_page
 *   DESCRIPTION: Marks a page as present corresponding to the given virtual
 *                address and maps it to the given physical address with given
 *                flags. 4 KB pages will always be mapped within 0-4 MB of
 *                physical memory.
 *        INPUTS: virtual_loc - virtual address. Will be truncated to 4 MB
 *                aligned. For 4 kB pages, will be masked with 0x003FF000.
 *                phys_loc - physical address. Will be truncated to 4 MB
 *                aligned. For 4 kB pages, will be masked with 0x003FF000.
 *                read_write - set flag for page read/write permissions
 *                user - set flag for user access
 *                page_size - set for 4 MB page, unset for 4 KB page
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Maps page in PD or 0-4 MB PT. Flushes TLBs.
 */
void map_page(uint32_t virtual_loc, uint32_t phys_loc, uint8_t read_write, uint8_t user, uint8_t page_size)
{
    /* Determine PD and PT index based on virtual_loc */
    uint32_t pd_num = (virtual_loc & FLAG_MASK) >> BITS_TO_PD_IDX;
    uint32_t pt_num = ((virtual_loc & FLAG_MASK) >> BITS_TO_PT_IDX) & PT_MASK;

    /* Align physical address according to page size */
    uint32_t entry = (page_size) ? phys_loc & ALIGN_MB : phys_loc & FLAG_MASK;

    /* Apply appropriate flags to table entry */
    entry |= (page_size) ? PDE_PAGE_SIZE : 0;
    entry |= (read_write) ? PDE_READ_WRITE : 0;
    entry |= (user) ? PDE_USER_SUPERVISOR : 0;
    entry |= PDE_PRESENT;

    /* Set entry in appropriate table */
    if (page_size) {                    /* Set: 4 MB page */
        page_directory[pd_num] = entry;
    } else {                            /* Clear: 4 KB page */
        page_table_0[pt_num] = entry;
    }

    __flush_tlb();
}

/*
 * unmap_page
 *   DESCRIPTION: Marks 4 kB or 4 MB page at given address as not present in PD/appropriate PT
 *        INPUTS: virtual_loc - virtual address. Will be truncated to 4 kB or 4 MB aligned
 *                page_size - set to unmap a 4MB page, unset to unmap 4kB page
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Unmaps page in PD. Flushes TLBs.
 */
void unmap_page(uint32_t virtual_loc, uint8_t page_size) {
    uint32_t pd_num = (virtual_loc & FLAG_MASK) >> BITS_TO_PD_IDX;
    uint32_t pt_num = ((virtual_loc & FLAG_MASK) >> BITS_TO_PT_IDX) & PT_MASK;

    if (page_size) {                    /* Clear 4 MB page */
        page_directory[pd_num] = 0; 
    } else {                            /* Clear 4kB page */
        page_table_0[pt_num] = 0;
    }

    __flush_tlb();
}

/*
 * __flush_tlb
 *   DESCRIPTION: Flushes TLBs by rewriting CR3.
 *        INPUTS: none
 *       OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Flushes TLBs
 */
void __flush_tlb() {
    /* Flush TLB by writing to cr3 */
    asm volatile(   
        "movl   %cr3, %eax \n\
         movl   %eax, %cr3 \n"
    );
}

