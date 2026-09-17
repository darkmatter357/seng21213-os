#include "pmm.h"
#include "types.h"

#define MAX_MEMORY_FRAMES 8192

static uint8_t frame_bitmap[MAX_MEMORY_FRAMES / 8];

static uint32_t total_frames = 0;
static uint32_t used_frames = 0;

static void bitmap_set(uint32_t frame)
{
    frame_bitmap[frame / 8] |= (uint8_t)(1u << (frame % 8));
}

static void bitmap_clear(uint32_t frame)
{
    frame_bitmap[frame / 8] &= (uint8_t)~(1u << (frame % 8));
}

static int bitmap_test(uint32_t frame)
{
    return (frame_bitmap[frame / 8] & (uint8_t)(1u << (frame % 8))) != 0;
}

void pmm_init(void)
{
    total_frames = MAX_MEMORY_FRAMES;
    used_frames = total_frames;

    /* Start with every frame marked as used. */
    for (uint32_t i = 0; i < MAX_MEMORY_FRAMES / 8; i++) {
        frame_bitmap[i] = 0xFF;
    }

    /*
     * QEMU currently gives us 32 MiB.
     * Mark every 4 KiB frame as available.
     */
    for (uint32_t frame = 0; frame < MAX_MEMORY_FRAMES; frame++) {
        bitmap_clear(frame);
    }

    /*
     * Reserve the first 1 MiB.
     * This protects the bootloader, BIOS areas, VGA memory,
     * kernel loading area, and other low-memory structures.
     */
    for (uint32_t frame = 0; frame < 256; frame++) {
        bitmap_set(frame);
    }

    used_frames = 256;
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t frame = 256; frame < MAX_MEMORY_FRAMES; frame++) {
        if (!bitmap_test(frame)) {
            bitmap_set(frame);
            used_frames++;
            return frame * PMM_FRAME_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t frame)
{
    uint32_t frame_number = frame / PMM_FRAME_SIZE;

    if (frame_number < 256 || frame_number >= MAX_MEMORY_FRAMES) {
        return;
    }

    if (bitmap_test(frame_number)) {
        bitmap_clear(frame_number);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_used_frames(void)
{
    return used_frames;
}

uint32_t pmm_free_frames(void)
{
    return total_frames - used_frames;
}
