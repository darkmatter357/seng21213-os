#ifndef RAMDISK_H
#define RAMDISK_H

#include "types.h"

/*
 * Stage 4 RAM disk
 *
 * Total size: 1 MiB
 * Block size: 512 bytes
 */
#define RAMDISK_SIZE  (1024 * 1024)
#define RAMDISK_BLOCK_SIZE 512
#define RAMDISK_BLOCK_COUNT (RAMDISK_SIZE / RAMDISK_BLOCK_SIZE)

/*
 * Initialise the RAM disk.
 */
void ramdisk_init(void);

/*
 * Read one 512-byte block from the RAM disk.
 *
 * Returns 0 on success, -1 on invalid block number.
 */
int ramdisk_read(uint32_t block, void *buffer);

/*
 * Write one 512-byte block to the RAM disk.
 *
 * Returns 0 on success, -1 on invalid block number.
 */
int ramdisk_write(uint32_t block, const void *buffer);

/*
 * Clear the entire RAM disk.
 */
void ramdisk_clear(void);

/*
 * Return the number of blocks in the RAM disk.
 */
uint32_t ramdisk_block_count(void);

#endif
