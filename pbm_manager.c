#include "pbm_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Crea una imagen PBM en blanco.
PBMImage *pbm_create(int width, int height) {
    PBMImage *img = malloc(sizeof(PBMImage));
    if (!img) return NULL;
    img->width = width;
    img->height = height;
    img->pixels = calloc((size_t)width * height, sizeof(uint8_t));
    if (!img->pixels) {
        free(img);
        return NULL;
    }
    return img;
}

// Libera la memoria de la imagen.
void pbm_free(PBMImage *image) {
    if (!image) return;
    free(image->pixels);
    free(image);
}

// Guarda la imagen PBM en formato ASCII (P1).
int pbm_save(const PBMImage *image, const char *filepath) {
    if (!image || !filepath) return -1;
    FILE *fp = fopen(filepath, "w");
    if (!fp) return -1;
    fprintf(fp, "P1\n");
    fprintf(fp, "# PBM saved by pbm_manager\n");
    fprintf(fp, "%d %d\n", image->width, image->height);
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            int v = image->pixels[y * image->width + x];
            fprintf(fp, "%d ", v);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}

// Carga una imagen PBM ASCII (P1).
PBMImage *pbm_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *fp = fopen(filepath, "r");
    if (!fp) return NULL;

    // Leer magic number
    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1) { fclose(fp); return NULL; }
    if (strcmp(magic, "P1") != 0) { fclose(fp); return NULL; }

    // Saltar comentarios y espacios en blanco antes de dimensiones
    int c = fgetc(fp);
    while (c == '#' || c == '\n' || c == ' ' || c == '\r' || c == '\t') {
        if (c == '#') {
            // Saltar hasta fin de línea
            do {
                c = fgetc(fp);
            } while (c != '\n' && c != EOF);
        }
        c = fgetc(fp);
    }
    ungetc(c, fp);

    int width = 0, height = 0;
    if (fscanf(fp, "%d %d", &width, &height) != 2) { fclose(fp); return NULL; }

    PBMImage *img = pbm_create(width, height);
    if (!img) { fclose(fp); return NULL; }

    for (int i = 0; i < width * height; ++i) {
        int v;
        if (fscanf(fp, "%d", &v) != 1) {
            pbm_free(img);
            fclose(fp);
            return NULL;
        }
        img->pixels[i] = (v != 0);
    }

    fclose(fp);
    return img;
}

// Obtiene el valor de un píxel.
int pbm_get_pixel(const PBMImage *image, int x, int y) {
    if (!image || x < 0 || y < 0 || x >= image->width || y >= image->height)
        return -1;
    return image->pixels[y * image->width + x];
}

// Establece el valor de un píxel.
int pbm_set_pixel(PBMImage *image, int x, int y, int value) {
    if (!image || x < 0 || y < 0 || x >= image->width || y >= image->height)
        return -1;
    image->pixels[y * image->width + x] = (value != 0);
    return 0;
}
