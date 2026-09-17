#ifndef VMM_H
#define VMM_H

#include "types.h"

#define PAGE_SIZE 4096

void vmm_init(void);

uint32_t vmm_get_directory(void);

void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr);
void vmm_unmap_page(uint32_t virtual_addr);

uint32_t vmm_get_physical(uint32_t virtual_addr);

#endif
