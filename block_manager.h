#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include "pbm_manager.h"
#include <stdint.h>
#include <stddef.h>

// Maneja el bitmap de bloques sobre la imagen PBM

typedef struct {
    PBMImage *img;
    int offset_bit;        // Bit donde inicia el bitmap (en la imagen)
    int block_count;       // Total de bloques gestionados
} BlockManager;

// Inicializa el gestor (bitmap inicia en offset_bit, maneja block_count bloques)
void bm_init(BlockManager *bm, PBMImage *img, int offset_bit, int block_count);

// Marca el bloque como usado/libre
int bm_alloc(BlockManager *bm, int block_idx);
int bm_free(BlockManager *bm, int block_idx);

// Consulta si el bloque está ocupado (1), libre (0), o error (<0)
int bm_is_allocated(const BlockManager *bm, int block_idx);

// Busca y reserva el primer bloque libre, retorna su índice o -1 si no hay espacio
int bm_alloc_first(BlockManager *bm);

// Calcula checksum XOR de todo el bitmap de bloques
uint32_t bm_checksum(const BlockManager *bm);

#endif
