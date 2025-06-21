#include "directory.h"
#include <string.h>
#include <stdio.h>

void dir_init(Directory *dir) {
    memset(dir, 0, sizeof(*dir));
}

int dir_find(const Directory *dir, const char *name) {
    for (int i = 0; i < BWFS_MAX_FILES; ++i) {
        if (dir->entries[i].used && 
            strncmp(dir->entries[i].name, name, BWFS_FILENAME_MAXLEN) == 0) {
            return i;
        }
    }
    return -1;
}

int dir_create(Directory *dir, const char *name) {
    // Rechazar nombres vacíos
    if (name == NULL || name[0] == '\0') {
        return -1;
    }
    
    // Ya existe
    if (dir_find(dir, name) >= 0) return -1;
    
    // Busca libre
    for (int i = 0; i < BWFS_MAX_FILES; ++i) {
        if (!dir->entries[i].used) {
            memset(&dir->entries[i], 0, sizeof(DirEntry));
            strncpy(dir->entries[i].name, name, BWFS_FILENAME_MAXLEN - 1);
            dir->entries[i].name[BWFS_FILENAME_MAXLEN - 1] = '\0';
            dir->entries[i].used = 1;
            return i;
        }
    }
    return -1;
}

int dir_remove(Directory *dir, const char *name) {
    int idx = dir_find(dir, name);
    if (idx < 0) return -1;
    memset(&dir->entries[idx], 0, sizeof(DirEntry));
    return 0;
}

const DirEntry *dir_entry(const Directory *dir, int idx) {
    if (idx < 0 || idx >= BWFS_MAX_FILES) return NULL;
    return &dir->entries[idx];
}

DirEntry *dir_entry_mut(Directory *dir, int idx) {
    if (idx < 0 || idx >= BWFS_MAX_FILES) return NULL;
    return &dir->entries[idx];
}

void dir_serialize(const Directory *dir, uint8_t *out) {
    memcpy(out, dir, sizeof(Directory));
}

void dir_deserialize(Directory *dir, const uint8_t *in) {
    memcpy(dir, in, sizeof(Directory));
}

uint32_t dir_checksum(const Directory *dir) {
    uint32_t sum = 0xCAFEBABE; // Valor inicial único
    const uint8_t *bytes = (const uint8_t*)dir;
    for (size_t i = 0; i < sizeof(Directory); ++i)
        sum ^= bytes[i] + (uint32_t)i;
    return sum;
}