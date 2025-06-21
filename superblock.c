#include "superblock.h"
#include "pbm_manager.h"
#include <string.h>
#include "directory.h"

uint32_t sb_checksum(const Superblock *sb) {
    const uint32_t *arr = (const uint32_t *)sb;
    uint32_t sum = 0;
    // Todos los campos excepto el último (checksum)
    for (size_t i = 0; i < (sizeof(Superblock)/sizeof(uint32_t)) - 1; ++i)
        sum ^= arr[i] + (uint32_t)i;  // Mejoramos el checksum
    return sum;
}

void sb_init(Superblock *sb, int width, int height, int block_size,
             int block_count, int max_files, int max_blocks_per_file) {
    memset(sb, 0, sizeof(*sb));
    sb->magic               = BWFS_MAGIC;
    sb->width               = width;
    sb->height              = height;
    sb->block_size          = block_size;
    sb->block_count         = block_count;
    sb->max_files           = max_files;
    sb->max_blocks_per_file = max_blocks_per_file;
    
    // Calculamos los offsets correctamente
    size_t s_bits = sizeof(Superblock) * 8;
    size_t d_bits = max_files * sizeof(DirEntry) * 8;
    
    sb->bitmap_offset = s_bits;
    sb->dir_offset    = s_bits + block_count; // bitmap = 1 bit por bloque
    sb->data_offset   = s_bits + block_count + d_bits;
    
    sb->checksum      = sb_checksum(sb);
}

int sb_save(const Superblock *sb, PBMImage *img) {
    if (!sb || !img) return -1;
    const uint8_t *bytes = (const uint8_t*)sb;
    for (size_t b = 0; b < sizeof(Superblock); ++b) {
        for (int bit = 0; bit < 8; ++bit) {
            int value = (bytes[b] >> (7 - bit)) & 1;
            pbm_set_pixel(img, (b*8 + bit) % img->width, (b*8 + bit) / img->width, value);
        }
    }
    return 0;
}

int sb_load(Superblock *sb, const PBMImage *img) {
    if (!sb || !img) return -1;
    uint8_t *bytes = (uint8_t*)sb;
    for (size_t b = 0; b < sizeof(Superblock); ++b) {
        bytes[b] = 0;
        for (int bit = 0; bit < 8; ++bit) {
            int value = pbm_get_pixel(img, (b*8 + bit) % img->width, (b*8 + bit) / img->width);
            if (value < 0) return -1;
            bytes[b] |= (value << (7 - bit));
        }
    }
    if (sb->magic != BWFS_MAGIC || sb_checksum(sb) != sb->checksum)
        return -2;
    return 0;
}