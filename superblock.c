#include "superblock.h"
#include "pbm_manager.h"
#include <string.h>
#include "directory.h"

uint32_t sb_checksum(const Superblock *sb) {
    const uint8_t *bytes = (const uint8_t*)sb;
    uint32_t sum = 0x12345678;
    // Excluir el campo checksum (últimos 4 bytes)
    for (size_t i = 0; i < sizeof(Superblock) - sizeof(uint32_t); i++) {
        sum = (sum << 5) ^ bytes[i] ^ (sum >> 27);
    }
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
    sb->signature           = BWFS_SIGNATURE;

    // Calcular offsets
    size_t s_bits = sizeof(Superblock) * 8;
    size_t d_bits = max_files * sizeof(DirEntry) * 8;

    sb->bitmap_offset = s_bits;
    sb->dir_offset    = s_bits + block_count;
    sb->data_offset   = s_bits + block_count + d_bits;

    // Inicializar checksum del directorio
    sb->dir_checksum = 0;
    
    // Calcular checksum final
    sb->checksum = 0;
    sb->checksum = sb_checksum(sb);
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
    
    // Verificar magic number primero
    if (sb->magic != BWFS_MAGIC) {
        return -2;
    }
    
    // Verificar checksum
    uint32_t stored_checksum = sb->checksum;
    Superblock temp = *sb;
    temp.checksum = 0;
    uint32_t computed_checksum = sb_checksum(&temp);
    
    if (computed_checksum != stored_checksum) {
        return -3;
    }
    
    return 0;
}