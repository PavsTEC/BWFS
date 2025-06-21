#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include "pbm_manager.h"
#include <stdint.h>

#define BWFS_MAGIC 0x42574653u  // 'BWFS'
#define BWFS_MAX_FILES 128
#define BWFS_MAX_BLOCKS_PER_FILE 32
#define BWFS_FILENAME_MAXLEN 32
#define BWFS_SIGNATURE 0x12345678  // Nuevo valor para la firma de la imagen inicial

typedef struct {
    uint32_t magic;
    uint32_t signature;  // Firma para identificar la imagen inicial
    uint32_t width;
    uint32_t height;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t max_files;
    uint32_t max_blocks_per_file;
    uint32_t bitmap_offset;
    uint32_t dir_offset;
    uint32_t data_offset;
    uint32_t checksum;
    uint32_t dir_checksum;
} Superblock;

uint32_t sb_checksum(const Superblock *sb);
void sb_init(Superblock *sb, int width, int height, int block_size,
             int block_count, int max_files, int max_blocks_per_file);
int sb_save(const Superblock *sb, PBMImage *img);
int sb_load(Superblock *sb, const PBMImage *img);

#endif