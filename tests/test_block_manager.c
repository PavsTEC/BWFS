#include <assert.h>
#include "pbm_manager.h"
#include "block_manager.h"
#include <stdlib.h>
#include <stdio.h>


int main() {
    // Creamos una PBM pequeña de 128x64 = 8192 píxeles → 2 bloques de 4KB
    PBMImage *img = pbm_create(128, 64);
    BlockManager *bm = bm_init(img, 4096);
    assert(bm != NULL);
    assert(bm->total_blocks == 2);

    // Reservar primer bloque
    int b0 = bm_alloc(bm);
    assert(b0 == 0);
    assert(bm_is_allocated(bm, 0) == 1);

    // Reservar segundo bloque
    int b1 = bm_alloc(bm);
    assert(b1 == 1);
    assert(bm_is_allocated(bm, 1) == 1);

    // Ya no hay más bloques
    assert(bm_alloc(bm) == -1);

    // Liberar bloque 0
    assert(bm_free(bm, 0) == 0);
    assert(bm_is_allocated(bm, 0) == 0);

    // Reservar de nuevo → debe devolver 0
    assert(bm_alloc(bm) == 0);

    pbm_free(img);
    bm_destroy(bm);

    printf("✅ Tests de block_manager pasaron exitosamente.\n");
    return 0;
}
