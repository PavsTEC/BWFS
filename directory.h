#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
#include "superblock.h"  // para BWFS_MAX_FILES, BWFS_FILENAME_MAXLEN, etc.

typedef struct {
    char     name[BWFS_FILENAME_MAXLEN];
    uint32_t size;
    uint32_t checksum;
    uint32_t blocks[BWFS_MAX_BLOCKS_PER_FILE];
    uint32_t block_count;
    uint8_t  used;
    uint8_t  is_dir;            // 0 = archivo, 1 = directorio
} DirEntry;

typedef struct {
    DirEntry entries[BWFS_MAX_FILES];
    uint32_t max_entries;       // siempre igual a BWFS_MAX_FILES
} Directory;

// Inicializa la tabla (pone todo a 0)
void    dir_init(Directory *dir);

// Busca por nombre (archivo o dir), retorna índice o -1
int     dir_find(const Directory *dir, const char *name);

// Crea un archivo, retorna índice o -1
int     dir_create(Directory *dir, const char *name);

// Crea un directorio de un nivel (usa is_dir=1), retorna índice o -1
int     dir_mkdir(Directory *dir, const char *name);

// Elimina archivo o directorio por nombre (no revisa si dir está vacío), retorna 0 o -1
int     dir_remove(Directory *dir, const char *name);

// Renombra archivo o directorio, retorna índice o -1
int     dir_rename(Directory *dir, const char *oldname, const char *newname);

// Serializa/Deserializa la tabla entera a un buffer de bytes
void    dir_serialize(const Directory *dir, uint8_t *out);
void    dir_deserialize(Directory *dir, const uint8_t *in);

// Calcula checksum XOR de todos los bytes de la tabla
uint32_t dir_checksum(const Directory *dir);

// Acceso directo a las entradas
const DirEntry *dir_entry(const Directory *dir, int idx);
DirEntry       *dir_entry_mut(Directory *dir, int idx);

#endif // DIRECTORY_H
