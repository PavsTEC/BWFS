#ifndef PBM_MANAGER_H
#define PBM_MANAGER_H

#include <stdint.h>

// Tamaño por defecto (pero puede usarse pbm_create con otros tamaños)
#define PBM_DEFAULT_WIDTH  4096
#define PBM_DEFAULT_HEIGHT 4096

typedef struct {
    uint8_t *pixels;  // 1 byte por píxel: 0 = blanco, 1 = negro
    int width;
    int height;
} PBMImage;

// Carga una imagen PBM ASCII (P1). Retorna NULL en error.
PBMImage *pbm_load(const char *filepath);

// Crea una imagen PBM en blanco de tamaño width x height.
PBMImage *pbm_create(int width, int height);

// Guarda la imagen PBM en formato ASCII (P1). Retorna 0 en éxito, -1 en error.
int pbm_save(const PBMImage *image, const char *filepath);

// Libera la memoria de la imagen.
void pbm_free(PBMImage *image);

// Obtiene el valor de un píxel. Retorna 0 o 1, o -1 si coordenadas inválidas.
int pbm_get_pixel(const PBMImage *image, int x, int y);

// Establece el valor de un píxel (0 o 1). Retorna 0 en éxito, -1 en error.
int pbm_set_pixel(PBMImage *image, int x, int y, int value);

#endif // PBM_MANAGER_H
