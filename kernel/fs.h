#ifndef FS_H
#define FS_H

#include "types.h"

/*
 * Simple flat filesystem for the 1 MiB RAM disk.
 */

#define FS_MAGIC            0x53465331u
#define FS_BLOCK_SIZE       512
#define FS_BLOCK_COUNT      2048

#define FS_MAX_FILES        64
#define FS_FILENAME_MAX     32
#define FS_MAX_FILE_SIZE    4096

/*
 * RAM-disk layout
 *
 * Block 0       : Superblock
 * Block 1       : Block bitmap
 * Block 2       : Inode bitmap
 * Blocks 3 - 10 : Inode table
 * Blocks 11+    : File data
 */
#define FS_SUPERBLOCK_BLOCK     0
#define FS_BLOCK_BITMAP_BLOCK   1
#define FS_INODE_BITMAP_BLOCK   2
#define FS_INODE_TABLE_START    3
#define FS_INODE_TABLE_BLOCKS   8
#define FS_DATA_BLOCK_START     11

typedef struct {
    uint32_t magic;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t data_start;
    uint32_t inode_table_start;
    uint32_t inode_table_blocks;
    uint32_t block_bitmap_block;
    uint32_t inode_bitmap_block;
} fs_superblock_t;

typedef struct {
    uint32_t used;
    uint32_t size;
    uint32_t first_block;
    char name[FS_FILENAME_MAX];
} fs_inode_t;

void fs_init(void);

int fs_create(const char *name);
int fs_delete(const char *name);

int fs_open(const char *name);
int fs_close(int fd);

int fs_read(int fd, void *buffer, uint32_t size);
int fs_write(int fd, const void *buffer, uint32_t size);

int fs_list(void);
int fs_get_name(uint32_t index, char *buffer);

#endif
