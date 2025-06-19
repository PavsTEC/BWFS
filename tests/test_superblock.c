#include <assert.h>
#include <stdio.h>
#include "pbm_manager.h"
#include "superblock.h"

#define SB_MAGIC 0x42574653

int main() {
    // 1) Crear imagen lo suficientemente grande
    PBMImage *img = pbm_create(4096, 4096);
    assert(img != NULL);

    // 2) Definir y guardar superbloque
    Superblock sb = {
        .magic = SB_MAGIC,
        .version = 1,
        .total_blocks = 512,
        .block_size = 4096,
        .segment_count = 1
    };
    assert(sb_save(img, &sb, NULL) == 0);

    // 3) Cargar superbloque y verificar campos
    Superblock *loaded = sb_load(img, NULL);
    assert(loaded != NULL);
    assert(loaded->magic == sb.magic);
    assert(loaded->version == sb.version);
    assert(loaded->total_blocks == sb.total_blocks);
    assert(loaded->block_size == sb.block_size);
    assert(loaded->segment_count == sb.segment_count);

    // 4) Limpieza
    sb_free(loaded);
    pbm_free(img);

    printf("✅ Tests de superblock pasaron exitosamente.\n");
    return 0;
}
