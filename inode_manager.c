#include "inode_manager.h"
#include "pbm_manager.h"
#include <stdlib.h>
#include <string.h>

#define SUPERBLOCK_SIZE 4096  // Debe coincidir con el reservado en superblock

// Variables internas
static PBMImage *inode_image    = NULL;
static int       inode_offset   = 0;  // Bloque de inicio para i-nodos
static int       inode_count    = 0;  // Número total de i-nodos
static const int inode_size     = sizeof(Inode);

/**
 * Inicializa el módulo de i-nodos.
 */
void inode_init(PBMImage *image, int inode_region_offset, int max_inodes) {
    inode_image  = image;
    inode_offset = inode_region_offset;
    inode_count  = max_inodes;
}

/** Calcula el índice de píxel inicial para un i-nodo dado */
static int inode_pixel_start(int idx) {
    // Cada bloque está mapeado a SUPERBLOCK_SIZE píxeles
    return inode_offset * SUPERBLOCK_SIZE + idx * inode_size;
}

int inode_alloc(void) {
    if (!inode_image) return -1;
    for (int i = 0; i < inode_count; i++) {
        Inode tmp;
        if (inode_load(i, &tmp) != 0) return -1;
        // Consideramos libre si size == 0
        if (tmp.size == 0) {
            return i;
        }
    }
    return -1;
}

int inode_free(int inode_idx) {
    if (!inode_image || inode_idx < 0 || inode_idx >= inode_count) return -1;
    Inode zero = {0};
    return inode_save(inode_idx, &zero);
}

int inode_save(int inode_idx, const Inode *node) {
    if (!inode_image || !node || inode_idx < 0 || inode_idx >= inode_count) return -1;
    int start = inode_pixel_start(inode_idx);
    uint8_t buf[inode_size];
    memcpy(buf, node, inode_size);
    for (int i = 0; i < inode_size; i++) {
        inode_image->pixels[start + i] = buf[i];
    }
    return 0;
}

int inode_load(int inode_idx, Inode *out_node) {
    if (!inode_image || !out_node || inode_idx < 0 || inode_idx >= inode_count) return -1;
    int start = inode_pixel_start(inode_idx);
    uint8_t buf[inode_size];
    for (int i = 0; i < inode_size; i++) {
        buf[i] = inode_image->pixels[start + i];
    }
    memcpy(out_node, buf, inode_size);
    return 0;
}
