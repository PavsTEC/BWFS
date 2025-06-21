#include "pbm_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PBMImage *pbm_create(int width, int height) {
    PBMImage *img = malloc(sizeof(PBMImage));
    if (!img) return NULL;
    img->width = width;
    img->height = height;
    img->stride_bytes = (width + 7) / 8;
    img->bits = calloc(img->stride_bytes * height, 1);
    if (!img->bits) { free(img); return NULL; }
    return img;
}

void pbm_free(PBMImage *img) {
    if (img) { free(img->bits); free(img); }
}

int pbm_get_pixel(const PBMImage *img, int x, int y) {
    if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height) return -1;
    int byte_offset = y * img->stride_bytes + (x / 8);
    int bit_offset = 7 - (x % 8);
    return (img->bits[byte_offset] >> bit_offset) & 1;
}

int pbm_set_pixel(PBMImage *img, int x, int y, int value) {
    if (!img || x < 0 || y < 0 || x >= img->width || y >= img->height) return -1;
    int byte_offset = y * img->stride_bytes + (x / 8);
    int bit_offset = 7 - (x % 8);
    if (value)
        img->bits[byte_offset] |= (1 << bit_offset);
    else
        img->bits[byte_offset] &= ~(1 << bit_offset);
    return 0;
}

PBMImage *pbm_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    char magic[3];
    if (!fgets(magic, sizeof(magic), f) || strncmp(magic, "P4", 2) != 0) {
        fclose(f);
        return NULL;
    }

    int width = 0, height = 0;
    int c;
    do { c = fgetc(f); } while (c == '\n' || c == '\r' || c == '#');
    ungetc(c, f);

    if (fscanf(f, "%d %d", &width, &height) != 2 || width <= 0 || height <= 0) {
        fclose(f);
        return NULL;
    }
    fgetc(f);

    size_t stride = (width + 7) / 8;
    PBMImage *img = malloc(sizeof(PBMImage));
    if (!img) {
        fclose(f);
        return NULL;
    }
    img->width = width;
    img->height = height;
    img->stride_bytes = stride;
    img->bits = malloc(stride * height);
    if (!img->bits) {
        free(img);
        fclose(f);
        return NULL;
    }

    size_t read = fread(img->bits, 1, stride * height, f);
    if (read != stride * height) {
        free(img->bits);
        free(img);
        fclose(f);
        return NULL;
    }

    fclose(f);
    return img;
}

int pbm_save(const PBMImage *img, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "P4\n%d %d\n", img->width, img->height);
    fwrite(img->bits, 1, img->stride_bytes * img->height, f);
    fclose(f);
    return 0;
}