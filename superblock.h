#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#include <stdint.h>
#include "pbm_manager.h"

// Tamaño reservado para el superbloque: usamos un bloque de 4 KB (4096 bytes),
// asumiendo que image tiene al menos 4096 píxeles reservados para él.
#define SUPERBLOCK_SIZE 4096

typedef struct {
    uint32_t magic;          // Constante identificadora, p.ej. 0xBWF5
    uint32_t version;        // Versión del FS
    uint64_t total_blocks;   // Bloques totales en todos los segmentos
    uint32_t block_size;     // Tamaño en bytes de cada bloque (4096)
    uint32_t segment_count;  // Cuántas imágenes forman el FS
    // (El struct ocupa menos de SUPERBLOCK_SIZE bytes; el resto queda sin usar)
} Superblock;

// Serializa y guarda el superbloque en la imagen (bloque 0). Retorna 0 en éxito.
int sb_save(PBMImage *image, const Superblock *sb, const char *passphrase);

// Carga y deserializa el superbloque desde la imagen (bloque 0). Retorna NULL en error.
Superblock *sb_load(PBMImage *image, const char *passphrase);

// Libera la estructura Superblock* retornada por sb_load
void sb_free(Superblock *sb);

#endif // SUPERBLOCK_H
