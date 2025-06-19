#include "block_manager.h"
#include <stdlib.h>  // malloc, free, NULL

// Convierte un índice de bloque en coordenadas de píxel (x,y)
static void block_to_coord(const BlockManager *bm, int block_index, int *out_x, int *out_y) {
    long pixel_index = (long)block_index * bm->pixels_per_block;
    *out_y = pixel_index / bm->image->width;
    *out_x = pixel_index % bm->image->width;
}

BlockManager *bm_init(PBMImage *image, int block_size) {
    if (image == NULL || block_size <= 0) return NULL;
    BlockManager *bm = malloc(sizeof(BlockManager));
    if (bm == NULL) return NULL;
    bm->image = image;
    bm->block_size = block_size;
    bm->pixels_per_block = block_size;
    bm->total_blocks = (image->width * image->height) / bm->pixels_per_block;
    return bm;
}

void bm_destroy(BlockManager *bm) {
    free(bm);
}

int bm_alloc(BlockManager *bm) {
    if (bm == NULL) return -1;
    for (int i = 0; i < bm->total_blocks; i++) {
        int x, y;
        block_to_coord(bm, i, &x, &y);
        if (pbm_get_pixel(bm->image, x, y) == 0) {
            pbm_set_pixel(bm->image, x, y, 1);
            return i;
        }
    }
    return -1;
}

int bm_free(BlockManager *bm, int block_index) {
    if (bm == NULL ||
        block_index < 0 ||
        block_index >= bm->total_blocks) return -1;
    int x, y;
    block_to_coord(bm, block_index, &x, &y);
    pbm_set_pixel(bm->image, x, y, 0);
    return 0;
}

int bm_is_allocated(const BlockManager *bm, int block_index) {
    if (bm == NULL ||
        block_index < 0 ||
        block_index >= bm->total_blocks) return -1;
    int x, y;
    block_to_coord(bm, block_index, &x, &y);
    return pbm_get_pixel(bm->image, x, y);
}
