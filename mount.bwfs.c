// mount.bwfs.c
#define FUSE_USE_VERSION 31
#define _XOPEN_SOURCE 700

#include <fuse3/fuse.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/statvfs.h>
#include "fs_image.h"

static FSImage   *fs           = NULL;
static const char *fs_folder   = NULL;

// Helper: strip leading '/' and copy into buf (maxlen includes NUL)
static void strip_slash(const char *path, char *buf, size_t maxlen) {
    if (path[0]=='/') {
        strncpy(buf, path+1, maxlen-1);
    } else {
        strncpy(buf, path,   maxlen-1);
    }
    buf[maxlen-1] = '\0';
}

// Translate path to DirEntry*, NULL for root
static int resolve_path(const char *path, DirEntry **out) {
    if (strcmp(path, "/")==0) {
        *out = NULL;
        return 0;
    }
    char name[BWFS_FILENAME_MAXLEN+1];
    strip_slash(path, name, sizeof(name));
    int idx = dir_find(&fs->dir, name);
    if (idx < 0) return -ENOENT;
    *out = dir_entry_mut(&fs->dir, idx);
    return 0;
}

// getattr
static int bwfs_getattr(const char *path, struct stat *st,
                        struct fuse_file_info *fi)
{
    (void)fi;
    memset(st, 0, sizeof(*st));
    DirEntry *e;
    int rc = resolve_path(path, &e);
    if (rc<0) return rc;

    if (e==NULL) {
        // root
        st->st_mode  = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (e->is_dir) {
        st->st_mode  = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else {
        st->st_mode  = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size  = e->size;
    }
    return 0;
}

// readdir
static int bwfs_readdir(const char *path, void *buf,
                        fuse_fill_dir_t filler,
                        off_t offset,
                        struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags)
{
    (void)offset; (void)fi; (void)flags;
    DirEntry *d;
    int rc = resolve_path(path, &d);
    if (rc<0) return rc;
    if (d && !d->is_dir) return -ENOTDIR;

    filler(buf, ".",  NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    for (uint32_t i = 0; i < fs->dir.max_entries; i++) {
        DirEntry *e = &fs->dir.entries[i];
        if (!e->used) continue;
        if (d==NULL && e->is_dir) {
            filler(buf, e->name, NULL, 0, 0);
        } else if (d==NULL && !e->is_dir) {
            filler(buf, e->name, NULL, 0, 0);
        } else if (d!=NULL) {
            // single-level: children of root only
            // nothing
        }
    }
    return 0;
}

// create
static int bwfs_create(const char *path, mode_t mode,
                       struct fuse_file_info *fi)
{
    (void)mode; (void)fi;
    char name[BWFS_FILENAME_MAXLEN+1];
    strip_slash(path, name, sizeof(name));
    int rc = fs_create_file(fs, name);
    return rc<0 ? -EEXIST : 0;
}

// open
static int bwfs_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;//No se esta usando de momento
    DirEntry *e;
    int rc = resolve_path(path, &e);
    return rc;
}

// read
static int bwfs_read(const char *path, char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi)
{
    (void)fi;
    DirEntry *e;
    int rc = resolve_path(path, &e);
    if (rc<0) return rc;
    if (e->is_dir) return -EISDIR;

    // read full then copy slice
    uint8_t *tmp = malloc(e->size);
    if (!tmp) return -ENOMEM;
    ssize_t rd = fs_read_file(fs, e->name, tmp, e->size);
    if (rd < 0) { free(tmp); return rd; }
    size_t to_copy = (offset + size <= (size_t)rd)
                       ? size : (size_t)(rd - offset);
    if ((off_t)to_copy < 0) to_copy = 0;
    memcpy(buf, tmp + offset, to_copy);
    free(tmp);
    return to_copy;
}

// write
static int bwfs_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi)
{
    (void)fi;
    DirEntry *e;
    int rc = resolve_path(path, &e);
    if (rc == -ENOENT) {
        // auto-create
        char name[BWFS_FILENAME_MAXLEN+1];
        strip_slash(path, name, sizeof(name));
        rc = fs_create_file(fs, name);
        if (rc<0) return rc;
        resolve_path(path, &e);
    }
    if (e->is_dir) return -EISDIR;

    size_t new_size = offset + size;
    uint8_t *tmp = calloc(1, new_size);
    if (!tmp) return -ENOMEM;
    ssize_t rd = fs_read_file(fs, e->name, tmp, new_size);
    if (rd < 0) rd = 0;
    memcpy(tmp + offset, buf, size);
    ssize_t wr = fs_write_file(fs, e->name, tmp, new_size);
    free(tmp);
    return (wr<0) ? wr : (int)size;
}

// mkdir
static int bwfs_mkdir(const char *path, mode_t mode) {
    (void)mode;
    char name[BWFS_FILENAME_MAXLEN+1];
    strip_slash(path, name, sizeof(name));
    int rc = fs_mkdir(fs, name);
    return rc;
}

// unlink / rmdir
static int bwfs_unlink(const char *path) {
    char name[BWFS_FILENAME_MAXLEN+1];
    strip_slash(path, name, sizeof(name));
    // check if dir or file
    DirEntry *e;
    int rc = resolve_path(path, &e);
    if (rc<0) return rc;
    if (e->is_dir)
        return fs_rmdir(fs, name);
    else
        return fs_remove_file(fs, name);
}

// rename
static int bwfs_rename(const char *from, const char *to, unsigned int flags) {
    (void)flags;
    char oldn[BWFS_FILENAME_MAXLEN+1], newn[BWFS_FILENAME_MAXLEN+1];
    strip_slash(from, oldn, sizeof(oldn));
    strip_slash(to,   newn, sizeof(newn));
    return fs_rename(fs, oldn, newn);
}

// access
static int bwfs_access(const char *path, int mask) {
    (void)mask;
    if (strcmp(path,"/")==0) return 0;
    char name[BWFS_FILENAME_MAXLEN+1];
    strip_slash(path, name, sizeof(name));
    return fs_access(fs, name, mask);
}

// flush / fsync
static int bwfs_flush(const char *path, struct fuse_file_info *fi) {
    (void)path; (void)fi;
    return fs_save(fs, fs_folder)==0 ? 0 : -EIO;
}
static int bwfs_fsync(const char *path, int datasync,
                      struct fuse_file_info *fi)
{
    (void)datasync;//De momento no se usa
    return bwfs_flush(path, fi);
}

// statfs
static int bwfs_statfs(const char *path, struct statvfs *st) {
    (void)path;
    return fs_statfs(fs, st);
}

// lseek
static off_t bwfs_lseek(const char *path, off_t off, int whence,
                        struct fuse_file_info *fi)
{
    DirEntry *e;
    int rc = resolve_path(path, &e);
    if (rc<0) return rc;
    off_t newoff;
    if (whence==SEEK_SET) newoff = off;
    else if (whence==SEEK_CUR) newoff = fi->fh + off;
    else if (whence==SEEK_END) newoff = e->size + off;
    else return -EINVAL;
    if (newoff < 0) return -EINVAL;
    fi->fh = newoff;
    return newoff;
}

static const struct fuse_operations bwfs_ops = {
    .getattr = bwfs_getattr,
    .readdir = bwfs_readdir,
    .create  = bwfs_create,
    .open     = bwfs_open,
    .read     = bwfs_read,
    .write    = bwfs_write,
    .mkdir    = bwfs_mkdir,
    .unlink   = bwfs_unlink,
    .rename   = bwfs_rename,
    .access   = bwfs_access,
    .flush    = bwfs_flush,
    .fsync    = bwfs_fsync,
    .statfs   = bwfs_statfs,
    .lseek    = bwfs_lseek,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <bwfs_folder> <mount_point>\n", argv[0]);
        return 1;
    }
    fs_folder = argv[1];
    fs        = fs_load(fs_folder);
    if (!fs) {
        fprintf(stderr, "Error loading BWFS from '%s'\n", fs_folder);
        return 1;
    }
    // Build minimal argv for fuse_main
    char *fuse_argv[] = { argv[0], argv[2], NULL };
    int fuse_argc = 2;
    return fuse_main(fuse_argc, fuse_argv, &bwfs_ops, NULL);
}