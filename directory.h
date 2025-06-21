#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
#include "superblock.h"  // Usar constantes de aquí

typedef struct {
    char name[BWFS_FILENAME_MAXLEN];
    uint32_t size;
    uint32_t checksum;
    uint32_t blocks[BWFS_MAX_BLOCKS_PER_FILE];
    uint32_t block_count;
    uint8_t used;
} DirEntry;

typedef struct {
    DirEntry entries[BWFS_MAX_FILES];
} Directory;

// Inicializa (memset 0)
void dir_init(Directory *dir);

// Busca archivo por nombre, retorna índice o -1
int dir_find(const Directory *dir, const char *name);

// Crea nuevo archivo, retorna índice o -1
int dir_create(Directory *dir, const char *name);

// Elimina archivo por nombre, retorna 0 o -1
int dir_remove(Directory *dir, const char *name);

// Lee DirEntry por índice
const DirEntry *dir_entry(const Directory *dir, int idx);
DirEntry       *dir_entry_mut(Directory *dir, int idx);

// Serializa y deserializa la tabla de archivos (a bits)
void dir_serialize(const Directory *dir, uint8_t *out);
void dir_deserialize(Directory *dir, const uint8_t *in);

// Calcula checksum XOR de todos los bytes de la tabla de archivos
uint32_t dir_checksum(const Directory *dir);

#endif
