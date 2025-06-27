#ifndef FS_IMAGE_H
#define FS_IMAGE_H

#include <sys/types.h>
#include <sys/statvfs.h>    // para struct statvfs
#include "pbm_manager.h"
#include "superblock.h"
#include "block_manager.h"
#include "directory.h"

#define BWFS_SIGNATURE 0x12345678  // Firma de la imagen inicial

typedef struct {
    PBMImage   **images;    // Lista de imágenes PBM
    int          image_count;
    Superblock  sb;
    BlockManager bm;
    Directory    dir;
} FSImage;

// Creación, carga y destrucción
FSImage *fs_create(int width, int height, int block_size);
FSImage *fs_load(const char *folder_path);
void     fs_destroy(FSImage *fs);

// Persistencia
int  fs_save(   FSImage *fs, const char *folder_path);
void fs_update_checksums(FSImage *fs);

// Operaciones de archivo
int     fs_create_file(FSImage *fs, const char *name);
int     fs_remove_file(FSImage *fs, const char *name);
ssize_t fs_read_file(  FSImage *fs, const char *name, void *buf, size_t count);
ssize_t fs_write_file( FSImage *fs, const char *name, const void *buf, size_t count);

// Integridad
int     fs_check_integrity(FSImage *fs);

// Operaciones de directorio (nivel 1)
int     fs_mkdir(    FSImage *fs, const char *dirname);
int     fs_rmdir(    FSImage *fs, const char *dirname);
int     fs_rename(   FSImage *fs, const char *oldname, const char *newname);

// Métodos de fachada para FUSE
int     fs_access(   FSImage *fs, const char *name, int mask);
int     fs_statfs(   FSImage *fs, struct statvfs *st);

#endif // FS_IMAGE_H
