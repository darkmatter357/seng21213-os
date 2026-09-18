#include "ramdisk.h"

static uint8_t ramdisk[RAMDISK_SIZE];

void ramdisk_init(void)
{
    ramdisk_clear();
}

int ramdisk_read(uint32_t block, void *buffer)
{
    uint32_t offset;
    uint32_t i;
    uint8_t *dest;

    if (block >= RAMDISK_BLOCK_COUNT || buffer == (void *)0) {
        return -1;
    }

    offset = block * RAMDISK_BLOCK_SIZE;
    dest = (uint8_t *)buffer;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        dest[i] = ramdisk[offset + i];
    }

    return 0;
}

int ramdisk_write(uint32_t block, const void *buffer)
{
    uint32_t offset;
    uint32_t i;
    const uint8_t *src;

    if (block >= RAMDISK_BLOCK_COUNT || buffer == (const void *)0) {
        return -1;
    }

    offset = block * RAMDISK_BLOCK_SIZE;
    src = (const uint8_t *)buffer;

    for (i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        ramdisk[offset + i] = src[i];
    }

    return 0;
}

void ramdisk_clear(void)
{
    uint32_t i;

    for (i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }
}

uint32_t ramdisk_block_count(void)
{
    return RAMDISK_BLOCK_COUNT;
}
