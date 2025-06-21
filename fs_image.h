#ifndef FS_IMAGE_H
#define FS_IMAGE_H

#include <sys/types.h>  // Para ssize_t
#include "pbm_manager.h"
#include "superblock.h"
#include "block_manager.h"
#include "directory.h"

// Eliminamos las redefiniciones y usamos las de superblock/directory
typedef struct {
    PBMImage *img;
    Superblock sb;
    BlockManager bm;
    Directory dir;
} FSImage;

FSImage *fs_create(int width, int height, int block_size);
FSImage *fs_load(const char *path);
int fs_save(FSImage *fs, const char *path);
void fs_destroy(FSImage *fs);
int fs_create_file(FSImage *fs, const char *name);
int fs_remove_file(FSImage *fs, const char *name);
ssize_t fs_write_file(FSImage *fs, const char *name, const void *buf, size_t count);
ssize_t fs_read_file(FSImage *fs, const char *name, void *buf, size_t count);
int fs_check_integrity(FSImage *fs);

#endif