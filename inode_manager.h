#ifndef INODE_MANAGER_H
#define INODE_MANAGER_H

#include <stdint.h>
#include "pbm_manager.h"

#define INODE_DIRECT_POINTERS 12

typedef struct {
    uint32_t size;                          // Tamaño del archivo en bytes
    uint32_t direct[INODE_DIRECT_POINTERS]; // Punteros a bloques directos
    uint32_t indirect;                      // Puntero a bloque indirecto (opcional)
} Inode;

/**
 * Inicializa la gestión de i-nodos.
 * - image:      la PBM donde se almacenarán los i-nodos.
 * - inode_region_offset: índice de bloque donde empiezan los i-nodos.
 * - max_inodes: cuántos i-nodos caben.
 */
void inode_init(PBMImage *image, int inode_region_offset, int max_inodes);

/**
 * Reserva un i-nodo libre, devuelve su índice o -1 si no hay.
 */
int inode_alloc(void);

/**
 * Libera un i-nodo (por índice), devuelve 0 en éxito, -1 en error.
 */
int inode_free(int inode_idx);

/**
 * Guarda un Inode en disco (región de i-nodos).
 * Devuelve 0 en éxito, -1 en error.
 */
int inode_save(int inode_idx, const Inode *node);

/**
 * Carga un Inode de disco a out_node.
 * Devuelve 0 en éxito, -1 en error.
 */
int inode_load(int inode_idx, Inode *out_node);

#endif // INODE_MANAGER_H
