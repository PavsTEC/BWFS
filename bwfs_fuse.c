#define FUSE_USE_VERSION 31

#include <sys/stat.h>      // S_IFDIR, S_IFREG, etc.
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>        // getcwd(), opcional
#include <fuse3/fuse.h>    // o <fuse/fuse.h> si usas FUSE2

#include "fs_image.h"

// RUTAS ABSOLUTAS a tus PBM (para evitar problemas de chdir)
static const char *FS_PATHS[] = {
    "/home/pablo/Desktop/BWFS/seg0.pbm",
    "/home/pablo/Desktop/BWFS/seg1.pbm"
};
static const int   FS_SEGMENTS = 2;
static const char *FS_PASS     = "clave123";

static FSImage *fs;

// Permitimos todos los accesos
static int bwfs_access(const char *path, int mask) {
    (void)path; (void)mask;
    return 0;
}

// Init vuelve a tu cwd original antes de cargar
static void *bwfs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void)conn; (void)cfg;
    fs = fs_load(FS_PATHS, FS_SEGMENTS, FS_PASS);
    if (!fs) {
        fprintf(stderr, "Error cargando BWFS (fs_load devolvió NULL)\n");
    }
    return fs;
}

static void bwfs_destroy(void *private_data) {
    (void)private_data;
    fs_destroy(fs);
}

// Aquí marcamos la raíz como tuya (UID/GID de quien monta)
static int bwfs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void)fi;
    memset(st, 0, sizeof(*st));
    if (strcmp(path, "/") == 0) {
        st->st_mode  = __S_IFDIR | 0755;
        st->st_nlink = 2;
        // asigna propietario al UID/GID que montó el FS
        struct fuse_context *ctx = fuse_get_context();
        st->st_uid = ctx->uid;
        st->st_gid = ctx->gid;
        return 0;
    }
    return -ENOENT;
}

static int bwfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void)offset; (void)fi; (void)flags;
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    filler(buf, ".",  NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    return 0;
}

static struct fuse_operations bwfs_ops = {
    .init      = bwfs_init,
    .destroy   = bwfs_destroy,
    .access    = bwfs_access,
    .getattr   = bwfs_getattr,
    .readdir   = bwfs_readdir,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &bwfs_ops, NULL);
}
