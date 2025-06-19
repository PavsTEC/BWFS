#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
#include "pbm_manager.h"

#define DIR_NAME_MAX 28

typedef struct {
    uint32_t inode;               // Índice de i-nodo
    char     name[DIR_NAME_MAX];  // Nombre (no termina en ‘\0’ necesariamente si ocupa todo)
} DirEntry;

/**
 * Inicializa la gestión de directorios.
 * - image:               la PBM donde se almacenan las tablas de directorio.
 * - dir_region_offset:   bloque donde empieza la región de directorio.
 * - max_entries:         cuántas entradas caben.
 */
void dir_init(PBMImage *image, int dir_region_offset, int max_entries);

/**
 * Agrega una entrada con (name → inode). Devuelve 0 en éxito, -1 en error o lleno.
 */
int dir_add(const char *name, uint32_t inode);

/**
 * Elimina la entrada con ese name. Devuelve 0 en éxito, -1 si no existe.
 */
int dir_remove(const char *name);

/**
 * Busca name y devuelve su inode, o -1 si no lo encuentra.
 */
int dir_lookup(const char *name);

/**
 * Lista todas las entradas: llena `names[i]` y `inodes[i]` hasta `max_count`.
 * Retorna el número de entradas encontradas.
 */
int dir_list(char names[][DIR_NAME_MAX], uint32_t inodes[], int max_count);

#endif // DIRECTORY_H
