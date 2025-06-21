#ifndef PBM_MANAGER_H
#define PBM_MANAGER_H

#include <stddef.h>  // Para size_t
#include <stdint.h>  // Para uint8_t

typedef struct {
    int width;
    int height;
    size_t stride_bytes;
    uint8_t *bits;
} PBMImage;

PBMImage *pbm_create(int width, int height);
void pbm_free(PBMImage *img);
int pbm_get_pixel(const PBMImage *img, int x, int y);
int pbm_set_pixel(PBMImage *img, int x, int y, int value);
PBMImage *pbm_load(const char *filename);
int pbm_save(const PBMImage *img, const char *path);

#endif