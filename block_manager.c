#include "block_manager.h"
#include <string.h>

void bm_init(BlockManager *bm, PBMImage *img, int offset_bit, int block_count) {
    if (!img || block_count <= 0 || offset_bit < 0) return;
    bm->img = img;
    bm->offset_bit = offset_bit;
    bm->block_count = block_count;
}

int bm_alloc(BlockManager *bm, int block_idx) {
    if (!bm || block_idx < 0 || block_idx >= bm->block_count) return -1;
    int bitpos = bm->offset_bit + block_idx;
    int byte_offset = bitpos / 8;
    int bit_offset = 7 - (bitpos % 8);
    bm->img->bits[byte_offset] |= (1 << bit_offset);
    return 0;
}

int bm_free(BlockManager *bm, int block_idx) {
    if (!bm || block_idx < 0 || block_idx >= bm->block_count) return -1;
    int bitpos = bm->offset_bit + block_idx;
    int byte_offset = bitpos / 8;
    int bit_offset = 7 - (bitpos % 8);
    bm->img->bits[byte_offset] &= ~(1 << bit_offset);
    return 0;
}

int bm_is_allocated(const BlockManager *bm, int block_idx) {
    if (!bm || block_idx < 0 || block_idx >= bm->block_count) return -1;
    int bitpos = bm->offset_bit + block_idx;
    int byte_offset = bitpos / 8;
    int bit_offset = 7 - (bitpos % 8);
    return (bm->img->bits[byte_offset] >> bit_offset) & 1;
}

int bm_alloc_first(BlockManager *bm) {
    for (int i = 0; i < bm->block_count; ++i) {
        if (bm_is_allocated(bm, i) == 0) {
            bm_alloc(bm, i);
            return i;
        }
    }
    return -1;
}

uint32_t bm_checksum(const BlockManager *bm) {
    uint32_t sum = 0xDEADBEEF; // Valor inicial único
    for (int i = 0; i < bm->block_count; ++i) {
        int val = bm_is_allocated(bm, i);
        if (val < 0) continue;
        sum ^= (uint32_t)val + i;
    }
    return sum;
}