#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include "pbm_manager.h"

typedef struct {
    PBMImage   *image;           // Imagen que actúa de "disco"
    int         total_blocks;    // Número total de bloques en la imagen
    int         block_size;      // Bytes por bloque (e.g. 4096)
    int         pixels_per_block;// 64×64 = 4096 píxeles
} BlockManager;

/**
 * Inicializa un BlockManager sobre una imagen PBM existente.
 *  - image: la PBM donde actúa el bitmap (debe tener al menos pixels_per_block * n bloques).
 *  - block_size: número de bytes por bloque (4096).
 * Devuelve NULL en error.
 */
BlockManager *bm_init(PBMImage *image, int block_size);

/** Libera recursos del BlockManager (no libera la PBMImage). */
void bm_destroy(BlockManager *bm);

/**
 * Reserva el siguiente bloque libre.
 * Devuelve el índice del bloque (0..total_blocks-1) o -1 si no hay espacios.
 */
int bm_alloc(BlockManager *bm);

/** Marca el bloque dado como libre (0). */
int bm_free(BlockManager *bm, int block_index);

/** Consulta si un bloque está ocupado (1) o libre (0). */
int bm_is_allocated(const BlockManager *bm, int block_index);

#endif // BLOCK_MANAGER_H
