#ifndef FS_IMAGE_H
#define FS_IMAGE_H

#include "pbm_manager.h"
#include "block_manager.h"
#include "superblock.h"

typedef struct {
    PBMImage      **images;            // Array de segmentos
    BlockManager  **bms;               // BlockManagers por segmento
    Superblock     *sb;                // Superbloque en memoria
    int             segment_count;     // Nº de segmentos
    int             blocks_per_segment;// Bloques por segmento
    int             total_blocks;      // Bloques totales
} FSImage;

/**
 * Crea un FS nuevo en memoria.
 * - segment_count: cuántos segmentos
 * - width, height: tamaño de cada PBM
 * - block_size: bytes por bloque
 * - passphrase: si no está vacía, persiste y luego carga el SB
 */
FSImage *fs_create(int segment_count,
                   int width,
                   int height,
                   int block_size,
                   const char *passphrase);

/**
 * Carga un FS existente desde archivos PBM.
 * - paths: array de rutas a los PBM
 * - segment_count: cuántos paths hay
 * - passphrase: para descifrar superbloque
 */
FSImage *fs_load(const char * const *paths,
                 int segment_count,
                 const char *passphrase);

/** Destruye el FS y libera recursos. */
int fs_destroy(FSImage *fs);

/** Reserva el siguiente bloque libre global. */
int fs_alloc_block(FSImage *fs);

/** Libera un bloque global. */
int fs_free_block(FSImage *fs, int block_id);

/** Consulta si un bloque global está ocupado. */
int fs_is_allocated(FSImage *fs, int block_id);

#endif // FS_IMAGE_H
