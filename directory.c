#include "directory.h"
#include <string.h>
#include <stdio.h>

void dir_init(Directory *dir) {
    memset(dir, 0, sizeof(*dir));
    dir->max_entries = BWFS_MAX_FILES;
}

int dir_find(const Directory *dir, const char *name) {
    for (int i = 0; i < (int)dir->max_entries; ++i) {
        if (dir->entries[i].used &&
            strncmp(dir->entries[i].name, name, BWFS_FILENAME_MAXLEN) == 0) {
            return i;
        }
    }
    return -1;
}

int dir_create(Directory *dir, const char *name) {
    if (!name || name[0] == '\0') return -1;
    if (dir_find(dir, name) >= 0) return -1;
    for (int i = 0; i < (int)dir->max_entries; ++i) {
        DirEntry *e = &dir->entries[i];
        if (!e->used) {
            memset(e, 0, sizeof(*e));
            strncpy(e->name, name, BWFS_FILENAME_MAXLEN-1);
            e->name[BWFS_FILENAME_MAXLEN-1] = '\0';
            e->used    = 1;
            e->is_dir  = 0;
            return i;
        }
    }
    return -1;
}

int dir_mkdir(Directory *dir, const char *name) {
    if (!name || name[0] == '\0') return -1;
    if (dir_find(dir, name) >= 0) return -1;
    for (int i = 0; i < (int)dir->max_entries; ++i) {
        DirEntry *e = &dir->entries[i];
        if (!e->used) {
            memset(e, 0, sizeof(*e));
            strncpy(e->name, name, BWFS_FILENAME_MAXLEN-1);
            e->name[BWFS_FILENAME_MAXLEN-1] = '\0';
            e->used    = 1;
            e->is_dir  = 1;
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

int dir_rename(Directory *dir, const char *oldname, const char *newname) {
    if (!oldname || !newname || newname[0] == '\0') return -1;
    int idx_old = dir_find(dir, oldname);
    if (idx_old < 0) return -1;
    if (dir_find(dir, newname) >= 0) return -1;
    DirEntry *e = &dir->entries[idx_old];
    strncpy(e->name, newname, BWFS_FILENAME_MAXLEN-1);
    e->name[BWFS_FILENAME_MAXLEN-1] = '\0';
    return idx_old;
}

void dir_serialize(const Directory *dir, uint8_t *out) {
    memcpy(out, dir, sizeof(Directory));
}

void dir_deserialize(Directory *dir, const uint8_t *in) {
    memcpy(dir, in, sizeof(Directory));
    dir->max_entries = BWFS_MAX_FILES;
}

uint32_t dir_checksum(const Directory *dir) {
    uint32_t sum = 0xCAFEBABE;
    const uint8_t *bytes = (const uint8_t *)dir;
    for (size_t i = 0; i < sizeof(Directory); ++i)
        sum ^= bytes[i] + (uint32_t)i;
    return sum;
}

const DirEntry *dir_entry(const Directory *dir, int idx) {
    if (idx < 0 || idx >= (int)dir->max_entries) return NULL;
    return &dir->entries[idx];
}

DirEntry *dir_entry_mut(Directory *dir, int idx) {
    if (idx < 0 || idx >= (int)dir->max_entries) return NULL;
    return &dir->entries[idx];
}
