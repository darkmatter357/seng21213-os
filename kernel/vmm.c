#include "vmm.h"
#include "pmm.h"

#define PAGE_PRESENT  0x001
#define PAGE_WRITABLE 0x002

#define PAGE_ENTRIES 1024

/*
 * This initial VMM implementation manages the first 4 MiB
 * using one page directory and one page table.
 *
 * Both structures are page-aligned.
 */
static uint32_t page_directory[PAGE_ENTRIES]
    __attribute__((aligned(PAGE_SIZE)));

static uint32_t page_table[PAGE_ENTRIES]
    __attribute__((aligned(PAGE_SIZE)));

static uint32_t vmm_directory_phys = 0;

void vmm_init(void)
{
    uint32_t i;

    /*
     * Clear the page directory.
     */
    for (i = 0; i < PAGE_ENTRIES; i++) {
        page_directory[i] = 0;
        page_table[i] = 0;
    }

    /*
     * Identity-map the first 4 MiB.
     *
     * Virtual address == physical address.
     */
    for (i = 0; i < PAGE_ENTRIES; i++) {
        page_table[i] = (i * PAGE_SIZE) |
                        PAGE_PRESENT |
                        PAGE_WRITABLE;
    }

    /*
     * Page directory entry 0 points to the page table.
     */
    page_directory[0] = ((uint32_t)page_table) |
                        PAGE_PRESENT |
                        PAGE_WRITABLE;

    vmm_directory_phys = (uint32_t)page_directory;

    /*
     * Load the page directory into CR3.
     * Paging is not enabled here yet.
     */
    __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r"(vmm_directory_phys)
        : "memory"
    );
}

uint32_t vmm_get_directory(void)
{
    return vmm_directory_phys;
}

void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr)
{
    uint32_t directory_index;
    uint32_t table_index;

    directory_index = virtual_addr >> 22;
    table_index = (virtual_addr >> 12) & 0x3FF;

    /*
     * This initial implementation supports only
     * the first 4 MiB address range.
     */
    if (directory_index != 0) {
        return;
    }

    page_table[table_index] = (physical_addr & 0xFFFFF000) |
                              PAGE_PRESENT |
                              PAGE_WRITABLE;
}

void vmm_unmap_page(uint32_t virtual_addr)
{
    uint32_t directory_index;
    uint32_t table_index;

    directory_index = virtual_addr >> 22;
    table_index = (virtual_addr >> 12) & 0x3FF;

    if (directory_index != 0) {
        return;
    }

    page_table[table_index] = 0;
}

uint32_t vmm_get_physical(uint32_t virtual_addr)
{
    uint32_t directory_index;
    uint32_t table_index;
    uint32_t offset;

    directory_index = virtual_addr >> 22;
    table_index = (virtual_addr >> 12) & 0x3FF;
    offset = virtual_addr & 0xFFF;

    if (directory_index != 0) {
        return 0;
    }

    if ((page_table[table_index] & PAGE_PRESENT) == 0) {
        return 0;
    }

    return (page_table[table_index] & 0xFFFFF000) | offset;
}
