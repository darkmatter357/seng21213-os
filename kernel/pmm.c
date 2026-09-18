#include "pmm.h"
#include "types.h"

#define MAX_MEMORY_FRAMES 8192
#define RESERVED_LOW_FRAMES 256

/*
 * One bit represents one 4 KiB physical frame.
 * 8192 frames = 32 MiB maximum.
 */
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
    return (frame_bitmap[frame / 8] &
            (uint8_t)(1u << (frame % 8))) != 0;
}

/*
 * Mark an E820 usable memory range as free.
 *
 * This implementation targets the 32 MiB memory configured for QEMU.
 * E820 entries whose address is above the 32-bit low range are ignored.
 */
static void mark_usable_range(uint32_t base,
                              uint32_t length)
{
    uint32_t start;
    uint32_t end;
    uint32_t first_frame;
    uint32_t last_frame;
    uint32_t frame;

    if (length == 0) {
        return;
    }

    start = base;
    end = base + length;

    /* Protect against 32-bit address overflow. */
    if (end < start) {
        end = 0xFFFFFFFFu;
    }

    /* Limit PMM to the configured 32 MiB range. */
    if (start >= (MAX_MEMORY_FRAMES * PMM_FRAME_SIZE)) {
        return;
    }

    if (end > (MAX_MEMORY_FRAMES * PMM_FRAME_SIZE)) {
        end = MAX_MEMORY_FRAMES * PMM_FRAME_SIZE;
    }

    /*
     * Round the beginning upward and the end downward so that
     * only complete 4 KiB frames are released.
     */
    first_frame = (start + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
    last_frame = end / PMM_FRAME_SIZE;

    for (frame = first_frame; frame < last_frame; frame++) {
        if (bitmap_test(frame)) {
            bitmap_clear(frame);
            used_frames--;
        }
    }
}

void pmm_init(void)
{
    e820_map_t *map = (e820_map_t *)E820_MAP_ADDR;
    uint32_t highest_address = 0;
    uint32_t i;

    /*
     * Start with every possible frame marked as used.
     */
    for (i = 0; i < MAX_MEMORY_FRAMES / 8; i++) {
        frame_bitmap[i] = 0xFF;
    }

    /*
     * Determine the highest physical address reported by E820.
     * We only manage the first 32 MiB.
     */
    for (i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        e820_entry_t *entry = &map->entries[i];

        if (entry->base_high != 0 || entry->length_high != 0) {
            /*
             * The current QEMU configuration is below 4 GiB.
             * Ignore entries requiring addresses outside our 32-bit
             * low-address PMM range.
             */
            if (entry->base_high != 0) {
                continue;
            }
        }

        if (entry->base_low + entry->length_low >
            highest_address) {
            highest_address =
                entry->base_low + entry->length_low;
        }
    }

    /*
     * If E820 reports no usable range, keep a safe zero-sized PMM.
     */
    if (highest_address == 0) {
        total_frames = 0;
        used_frames = 0;
        return;
    }

    /*
     * Cap the PMM at 32 MiB.
     */
    if (highest_address >
        MAX_MEMORY_FRAMES * PMM_FRAME_SIZE) {
        highest_address =
            MAX_MEMORY_FRAMES * PMM_FRAME_SIZE;
    }

    total_frames =
        (highest_address + PMM_FRAME_SIZE - 1) /
        PMM_FRAME_SIZE;

    if (total_frames > MAX_MEMORY_FRAMES) {
        total_frames = MAX_MEMORY_FRAMES;
    }

    /*
     * Initially all frames below total_frames are considered used.
     */
    used_frames = total_frames;

    /*
     * Release only E820 type-1 (usable RAM) regions.
     */
    for (i = 0; i < map->count && i < E820_MAX_ENTRIES; i++) {
        e820_entry_t *entry = &map->entries[i];

        if (entry->type != 1) {
            continue;
        }

        if (entry->base_high != 0) {
            continue;
        }

        mark_usable_range(
            entry->base_low,
            entry->length_low
        );
    }

    /*
     * Reserve the first 1 MiB.
     *
     * This protects the BIOS area, bootloader, E820 map,
     * kernel loading area, VGA memory, and other low-memory data.
     */
    for (i = 0; i < RESERVED_LOW_FRAMES && i < total_frames; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_frames++;
        }
    }
}

uint32_t pmm_alloc_frame(void)
{
    uint32_t frame;

    for (frame = RESERVED_LOW_FRAMES;
         frame < total_frames;
         frame++) {

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

    if (frame_number < RESERVED_LOW_FRAMES ||
        frame_number >= total_frames) {
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
