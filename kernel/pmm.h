#ifndef PMM_H
#define PMM_H

#include "types.h"

/*
 * BIOS E820 memory map entry.
 *
 * The bootloader stores these entries at physical address 0x8000
 * before entering protected mode.
 */
typedef struct {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t acpi;
} e820_entry_t;

/* Location of the E820 handoff structure. */
#define E820_MAP_ADDR   0x8000

/* Maximum number of entries we support. */
#define E820_MAX_ENTRIES 16

typedef struct {
    uint16_t count;
    uint16_t reserved;
    e820_entry_t entries[E820_MAX_ENTRIES];
} e820_map_t;

/* Physical frame size: 4 KiB. */
#define PMM_FRAME_SIZE 4096

void pmm_init(void);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t frame);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

#endif
