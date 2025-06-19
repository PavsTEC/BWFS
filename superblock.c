#include "superblock.h"
#include "pbm_manager.h"
#include <stdlib.h>
#include <stdint.h>

// Guarda el superbloque bit a bit en los últimos sizeof(Superblock)*8 píxeles
int sb_save(PBMImage *image, const Superblock *sb, const char *passphrase) {
    (void)passphrase;  // No usamos cifrado en este ejemplo
    if (!image || !sb) return -1;

    int width    = image->width;
    int height   = image->height;
    int capacity = width * height;
    int sb_bytes = sizeof(Superblock);
    int required = sb_bytes * 8;  // 8 píxeles por byte

    if (capacity < required) return -1;

    const uint8_t *data = (const uint8_t*)sb;
    // Base al final de la imagen
    int base = capacity - required;
    for (int i = 0; i < sb_bytes; i++) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            int pix_idx = base + i * 8 + bit;
            int x = pix_idx % width;
            int y = pix_idx / width;
            int v = (byte >> bit) & 1;
            pbm_set_pixel(image, x, y, v);
        }
    }
    return 0;
}

// Carga el superbloque leyendo bit a bit de los últimos sizeof(Superblock)*8 píxeles
Superblock *sb_load(PBMImage *image, const char *passphrase) {
    (void)passphrase;
    if (!image) return NULL;

    int width    = image->width;
    int height   = image->height;
    int capacity = width * height;
    int sb_bytes = sizeof(Superblock);
    int required = sb_bytes * 8;

    if (capacity < required) return NULL;

    Superblock *sb = malloc(sizeof(Superblock));
    if (!sb) return NULL;

    uint8_t *data = (uint8_t*)sb;
    int base = capacity - required;
    for (int i = 0; i < sb_bytes; i++) {
        uint8_t byte = 0;
        for (int bit = 0; bit < 8; bit++) {
            int pix_idx = base + i * 8 + bit;
            int x = pix_idx % width;
            int y = pix_idx / width;
            int v = pbm_get_pixel(image, x, y);
            if (v) byte |= (1 << bit);
        }
        data[i] = byte;
    }
    return sb;
}

void sb_free(Superblock *sb) {
    free(sb);
}
