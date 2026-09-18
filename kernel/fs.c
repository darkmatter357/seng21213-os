#include "fs.h"
#include "ramdisk.h"

#define FS_BLOCK_BITMAP_BYTES 256
#define FS_INODE_BITMAP_BYTES 8

static uint8_t block_bitmap[FS_BLOCK_BITMAP_BYTES];
static uint8_t inode_bitmap[FS_INODE_BITMAP_BYTES];

static fs_superblock_t superblock;
static fs_inode_t inodes[FS_MAX_FILES];

static int open_files[FS_MAX_FILES];

static uint8_t fs_block_buffer[FS_BLOCK_SIZE];

static void fs_bitmap_set(uint8_t *bitmap, uint32_t index)
{
    bitmap[index / 8] |= (uint8_t)(1u << (index % 8));
}

static void fs_bitmap_clear(uint8_t *bitmap, uint32_t index)
{
    bitmap[index / 8] &= (uint8_t)~(1u << (index % 8));
}

static int fs_bitmap_test(const uint8_t *bitmap, uint32_t index)
{
    return (bitmap[index / 8] &
            (uint8_t)(1u << (index % 8))) != 0;
}

static int fs_find_free_block(void)
{
    uint32_t block;

    for (block = FS_DATA_BLOCK_START;
         block < FS_BLOCK_COUNT;
         block++) {

        if (!fs_bitmap_test(block_bitmap, block)) {
            return (int)block;
        }
    }

    return -1;
}

static void fs_mark_block_used(uint32_t block)
{
    fs_bitmap_set(block_bitmap, block);
}

static void fs_mark_block_free(uint32_t block)
{
    fs_bitmap_clear(block_bitmap, block);
}

static int fs_find_free_inode(void)
{
    uint32_t inode;

    for (inode = 0; inode < FS_MAX_FILES; inode++) {
        if (!fs_bitmap_test(inode_bitmap, inode)) {
            return (int)inode;
        }
    }

    return -1;
}

static void fs_mark_inode_used(uint32_t inode)
{
    fs_bitmap_set(inode_bitmap, inode);
}

static void fs_mark_inode_free(uint32_t inode)
{
    fs_bitmap_clear(inode_bitmap, inode);
}

static void fs_copy_name(char *dest, const char *src)
{
    uint32_t i;

    for (i = 0; i < FS_FILENAME_MAX - 1; i++) {
        if (src[i] == '\0') {
            break;
        }

        dest[i] = src[i];
    }

    dest[i] = '\0';

    for (i = i + 1; i < FS_FILENAME_MAX; i++) {
        dest[i] = '\0';
    }
}

static int fs_name_equal(const char *a, const char *b)
{
    uint32_t i;

    for (i = 0; i < FS_FILENAME_MAX; i++) {
        if (a[i] != b[i]) {
            return 0;
        }

        if (a[i] == '\0') {
            return 1;
        }
    }

    return 1;
}

static int fs_find_inode(const char *name)
{
    uint32_t i;

    for (i = 0; i < FS_MAX_FILES; i++) {
        if (inodes[i].used &&
            fs_name_equal(inodes[i].name, name)) {
            return (int)i;
        }
    }

    return -1;
}
static void fs_write_inode_table(void)
{
    uint32_t block;
    uint32_t offset;
    uint32_t i;
    uint32_t remaining;
    uint8_t *src;

    src = (uint8_t *)inodes;

    for (block = 0; block < FS_INODE_TABLE_BLOCKS; block++) {

        for (i = 0; i < FS_BLOCK_SIZE; i++) {
            fs_block_buffer[i] = 0;
        }

        offset = block * FS_BLOCK_SIZE;

        if (offset < sizeof(inodes)) {
            remaining = sizeof(inodes) - offset;

            if (remaining > FS_BLOCK_SIZE) {
                remaining = FS_BLOCK_SIZE;
            }

            for (i = 0; i < remaining; i++) {
                fs_block_buffer[i] = src[offset + i];
            }
        }

        ramdisk_write(
            FS_INODE_TABLE_START + block,
            fs_block_buffer
        );
    }
}
void fs_init(void)
{
    uint32_t i;
    uint32_t j;

    ramdisk_init();

    superblock.magic = FS_MAGIC;
    superblock.block_count = ramdisk_block_count();
    superblock.inode_count = FS_MAX_FILES;
    superblock.data_start = FS_DATA_BLOCK_START;
    superblock.inode_table_start = FS_INODE_TABLE_START;
    superblock.inode_table_blocks = FS_INODE_TABLE_BLOCKS;
    superblock.block_bitmap_block = FS_BLOCK_BITMAP_BLOCK;
    superblock.inode_bitmap_block = FS_INODE_BITMAP_BLOCK;

    /*
     * Clear block and inode bitmaps.
     */
    for (i = 0; i < FS_BLOCK_BITMAP_BYTES; i++) {
        block_bitmap[i] = 0;
    }

    for (i = 0; i < FS_INODE_BITMAP_BYTES; i++) {
        inode_bitmap[i] = 0;
    }

    /*
     * Reserve filesystem metadata blocks.
     */
    for (i = 0; i < FS_DATA_BLOCK_START; i++) {
        fs_bitmap_set(block_bitmap, i);
    }

    /*
     * Clear inode table and open-file state.
     */
    for (i = 0; i < FS_MAX_FILES; i++) {
        inodes[i].used = 0;
        inodes[i].size = 0;
        inodes[i].first_block = 0;

        for (j = 0; j < FS_FILENAME_MAX; j++) {
            inodes[i].name[j] = '\0';
        }

        open_files[i] = 0;
    }

    /*
     * Clear the block buffer.
     */
    for (i = 0; i < FS_BLOCK_SIZE; i++) {
        fs_block_buffer[i] = 0;
    }

    /*
     * Write the superblock to block 0.
     */
    {
        uint8_t *src = (uint8_t *)&superblock;

        for (i = 0; i < sizeof(fs_superblock_t); i++) {
            fs_block_buffer[i] = src[i];
        }
    }

    ramdisk_write(FS_SUPERBLOCK_BLOCK, fs_block_buffer);

    /*
     * Write the block bitmap to block 1.
     */
    for (i = 0; i < FS_BLOCK_SIZE; i++) {
        fs_block_buffer[i] = 0;
    }

    for (i = 0; i < FS_BLOCK_BITMAP_BYTES; i++) {
        fs_block_buffer[i] = block_bitmap[i];
    }

    ramdisk_write(FS_BLOCK_BITMAP_BLOCK, fs_block_buffer);

    /*
     * Write the inode bitmap to block 2.
     */
    for (i = 0; i < FS_BLOCK_SIZE; i++) {
        fs_block_buffer[i] = 0;
    }

    for (i = 0; i < FS_INODE_BITMAP_BYTES; i++) {
        fs_block_buffer[i] = inode_bitmap[i];
    }

    ramdisk_write(FS_INODE_BITMAP_BLOCK, fs_block_buffer);
}

int fs_create(const char *name)
{
    int inode_index;
    int data_block;
    uint32_t i;

    if (name == (const char *)0 || name[0] == '\0') {
        return -1;
    }

    if (fs_find_inode(name) >= 0) {
        return -1;
    }

    inode_index = fs_find_free_inode();

    if (inode_index < 0) {
        return -1;
    }

    inodes[inode_index].used = 1;
    inodes[inode_index].size = 0;
    inodes[inode_index].first_block = 0;

    fs_mark_inode_used((uint32_t)inode_index);

    data_block = fs_find_free_block();

    if (data_block < 0) {
        fs_mark_inode_free((uint32_t)inode_index);
        inodes[inode_index].used = 0;
        return -1;
    }

    inodes[inode_index].first_block = (uint32_t)data_block;

    fs_mark_block_used((uint32_t)data_block);

    fs_copy_name(inodes[inode_index].name, name);

    /*
     * Clear the file's first data block.
     */
    for (i = 0; i < FS_BLOCK_SIZE; i++) {
        fs_block_buffer[i] = 0;
    }

    ramdisk_write(inodes[inode_index].first_block, fs_block_buffer);

    return inode_index;
}

int fs_delete(const char *name)
{
    int inode_index;
    uint32_t data_block;

    inode_index = fs_find_inode(name);

    if (inode_index < 0) {
        return -1;
    }

    if (open_files[inode_index]) {
        return -1;
    }

    data_block = inodes[inode_index].first_block;

    if (data_block >= FS_DATA_BLOCK_START &&
        data_block < FS_BLOCK_COUNT) {
        fs_mark_block_free(data_block);
    }

    fs_mark_inode_free((uint32_t)inode_index);

    inodes[inode_index].used = 0;
    inodes[inode_index].size = 0;
    inodes[inode_index].first_block = 0;
    inodes[inode_index].name[0] = '\0';

    return 0;
}

int fs_open(const char *name)
{
    int inode_index;

    inode_index = fs_find_inode(name);

    if (inode_index < 0) {
        return -1;
    }

    open_files[inode_index] = 1;

    return inode_index;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_FILES) {
        return -1;
    }

    if (!open_files[fd]) {
        return -1;
    }

    open_files[fd] = 0;

    return 0;
}

int fs_read(int fd, void *buffer, uint32_t size)
{
    uint32_t i;
    uint32_t count;
    uint8_t *dest;

    if (fd < 0 ||
        fd >= FS_MAX_FILES ||
        buffer == (void *)0 ||
        !open_files[fd]) {
        return -1;
    }

    if (size > inodes[fd].size) {
        count = inodes[fd].size;
    } else {
        count = size;
    }

    if (ramdisk_read(inodes[fd].first_block, fs_block_buffer) != 0) {
        return -1;
    }

    dest = (uint8_t *)buffer;

    for (i = 0; i < count; i++) {
        dest[i] = fs_block_buffer[i];
    }

    return (int)count;
}

int fs_write(int fd, const void *buffer, uint32_t size)
{
    uint32_t i;
    const uint8_t *src;

    if (fd < 0 ||
        fd >= FS_MAX_FILES ||
        buffer == (const void *)0 ||
        !open_files[fd]) {
        return -1;
    }

    if (size > FS_MAX_FILE_SIZE) {
        return -1;
    }

    /*
     * This implementation currently stores each file
     * in one 512-byte RAM-disk block.
     */
    if (size > FS_BLOCK_SIZE) {
        return -1;
    }

    src = (const uint8_t *)buffer;

    for (i = 0; i < FS_BLOCK_SIZE; i++) {
        fs_block_buffer[i] = 0;
    }

    for (i = 0; i < size; i++) {
        fs_block_buffer[i] = src[i];
    }

    if (ramdisk_write(inodes[fd].first_block, fs_block_buffer) != 0) {
        return -1;
    }

    inodes[fd].size = size;

    return (int)size;
}

int fs_get_name(uint32_t index, char *buffer)
{
    uint32_t i;

    if (index >= FS_MAX_FILES ||
        buffer == (char *)0 ||
        !inodes[index].used) {
        return -1;
    }

    for (i = 0; i < FS_FILENAME_MAX; i++) {
        buffer[i] = inodes[index].name[i];

        if (inodes[index].name[i] == '\0') {
            break;
        }
    }

    buffer[FS_FILENAME_MAX - 1] = '\0';

    return 0;
}

int fs_list(void)
{
    uint32_t i;
    int count = 0;

    for (i = 0; i < FS_MAX_FILES; i++) {
        if (inodes[i].used) {
            count++;
        }
    }

    return count;
}
