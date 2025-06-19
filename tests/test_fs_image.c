#include <assert.h>
#include <stdio.h>
#include "fs_image.h"
#include "superblock.h"

int main() {
    // 1) Test básico de asignación de bloques en FS pequeño
    FSImage *fs1 = fs_create(2, 16, 16, 16, "");
    assert(fs1);
    assert(fs1->segment_count == 2);
    assert(fs1->blocks_per_segment == (16*16)/16);
    assert(fs1->total_blocks == fs1->segment_count * fs1->blocks_per_segment);

    int b0 = fs_alloc_block(fs1);
    assert(b0 == 0);
    assert(fs_is_allocated(fs1, b0) == 1);
    int b1 = fs_alloc_block(fs1);
    assert(b1 == 1);
    assert(fs_free_block(fs1, b0) == 0);
    assert(fs_is_allocated(fs1, b0) == 0);
    int b0_new = fs_alloc_block(fs1);
    assert(b0_new == b0);
    fs_destroy(fs1);

    // 2) Test de persistencia de Superblock
    const char *pwd = "clave";
    // Elegimos imagen de 64×64 para caber el SB (64*64=4096 bytes)
    FSImage *fs2 = fs_create(1, 64, 64, 16, pwd);
    assert(fs2);
    // Cargamos SB directamente desde la imagen[0]
    Superblock *loaded = sb_load(fs2->images[0], pwd);
    assert(loaded);
    assert(loaded->magic == fs2->sb->magic);
    assert(loaded->version == fs2->sb->version);
    assert(loaded->total_blocks == fs2->sb->total_blocks);
    assert(loaded->block_size == fs2->sb->block_size);
    assert(loaded->segment_count == fs2->sb->segment_count);
    sb_free(loaded);
    fs_destroy(fs2);

    printf("✅ Tests de fs_image pasaron exitosamente.\n");
    return 0;
}
