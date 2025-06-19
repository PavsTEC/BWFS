// fs_image.c

#include "fs_image.h"
#include <stdlib.h>

#define FS_MAGIC 0x42574653

FSImage *fs_create(int segment_count,
                   int width,
                   int height,
                   int block_size,
                   const char *passphrase)
{
    if (segment_count <= 0 || width <= 0 || height <= 0 || block_size <= 0)
        return NULL;

    FSImage *fs = malloc(sizeof(FSImage));
    if (!fs) return NULL;

    fs->segment_count = segment_count;
    fs->images  = malloc(sizeof(PBMImage*) * segment_count);
    fs->bms     = malloc(sizeof(BlockManager*) * segment_count);
    if (!fs->images || !fs->bms) {
        free(fs->images);
        free(fs->bms);
        free(fs);
        return NULL;
    }

    // Crear segmentos y sus BlockManagers
    for (int i = 0; i < segment_count; i++) {
        fs->images[i] = pbm_create(width, height);
        if (!fs->images[i]) { fs_destroy(fs); return NULL; }
        fs->bms[i] = bm_init(fs->images[i], block_size);
        if (!fs->bms[i]) { fs_destroy(fs); return NULL; }
    }

    fs->blocks_per_segment = (width * height) / block_size;
    fs->total_blocks       = fs->blocks_per_segment * segment_count;

    // Crear superbloque en memoria
    fs->sb = malloc(sizeof(Superblock));
    if (!fs->sb) { fs_destroy(fs); return NULL; }
    fs->sb->magic         = FS_MAGIC;
    fs->sb->version       = 1;
    fs->sb->total_blocks  = fs->total_blocks;
    fs->sb->block_size    = block_size;
    fs->sb->segment_count = segment_count;

    // Persistir superbloque solo si se pasó passphrase no vacía
    if (passphrase && passphrase[0] != '\0') {
        if (sb_save(fs->images[0], fs->sb, passphrase) != 0) {
            fs_destroy(fs);
            return NULL;
        }
        // Recargar desde la imagen para garantizar consistencia
        Superblock *reloaded = sb_load(fs->images[0], passphrase);
        if (!reloaded) {
            fs_destroy(fs);
            return NULL;
        }
        sb_free(fs->sb);
        fs->sb = reloaded;
    }

    return fs;
}

FSImage *fs_load(const char * const *paths,
                 int segment_count,
                 const char *passphrase)
{
    (void)passphrase;
    if (!paths || segment_count <= 0) return NULL;

    FSImage *fs = malloc(sizeof(FSImage));
    if (!fs) return NULL;

    fs->segment_count = segment_count;
    fs->images  = malloc(sizeof(PBMImage*) * segment_count);
    fs->bms     = malloc(sizeof(BlockManager*) * segment_count);
    if (!fs->images || !fs->bms) {
        free(fs->images);
        free(fs->bms);
        free(fs);
        return NULL;
    }

    // Cargar primera imagen y superbloque
    fs->images[0] = pbm_load(paths[0]);
    if (!fs->images[0]) { fs_destroy(fs); return NULL; }
    fs->sb = sb_load(fs->images[0], passphrase);
    if (!fs->sb) { fs_destroy(fs); return NULL; }

    int width  = fs->images[0]->width;
    int height = fs->images[0]->height;
    fs->blocks_per_segment = (width * height) / fs->sb->block_size;
    fs->total_blocks       = fs->sb->total_blocks;

    // Cargar el resto de imágenes
    for (int i = 1; i < segment_count; i++) {
        fs->images[i] = pbm_load(paths[i]);
        if (!fs->images[i]) { fs_destroy(fs); return NULL; }
    }
    // Inicializar sus BlockManagers
    for (int i = 0; i < segment_count; i++) {
        fs->bms[i] = bm_init(fs->images[i], fs->sb->block_size);
        if (!fs->bms[i]) { fs_destroy(fs); return NULL; }
    }

    return fs;
}

int fs_destroy(FSImage *fs)
{
    if (!fs) return -1;
    for (int i = 0; i < fs->segment_count; i++) {
        free(fs->bms[i]);
        pbm_free(fs->images[i]);
    }
    free(fs->images);
    free(fs->bms);
    sb_free(fs->sb);
    free(fs);
    return 0;
}

int fs_alloc_block(FSImage *fs)
{
    if (!fs) return -1;
    for (int i = 0; i < fs->segment_count; i++) {
        int local = bm_alloc(fs->bms[i]);
        if (local >= 0)
            return i * fs->blocks_per_segment + local;
    }
    return -1;
}

int fs_free_block(FSImage *fs, int block_id)
{
    if (!fs || block_id < 0 || block_id >= fs->total_blocks)
        return -1;
    int seg   = block_id / fs->blocks_per_segment;
    int local = block_id % fs->blocks_per_segment;
    return bm_free(fs->bms[seg], local);
}

int fs_is_allocated(FSImage *fs, int block_id)
{
    if (!fs || block_id < 0 || block_id >= fs->total_blocks)
        return -1;
    int seg   = block_id / fs->blocks_per_segment;
    int local = block_id % fs->blocks_per_segment;
    return bm_is_allocated(fs->bms[seg], local);
}
