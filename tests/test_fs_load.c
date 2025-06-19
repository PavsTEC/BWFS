#include <assert.h>
#include <stdio.h>
#include "fs_image.h"

int main() {
    const char *pwd = "clave123";
    const char *paths[2] = { "seg0.pbm", "seg1.pbm" };

    // 1) Crear FS y asignar algunos bloques
    FSImage *fs0 = fs_create(2, 64, 64, 16, pwd);
    assert(fs0);
    int a = fs_alloc_block(fs0);
    int b = fs_alloc_block(fs0);
    assert(a == 0 && b == 1);
    // Persistir las imágenes a disco
    for (int i = 0; i < 2; i++)
        pbm_save(fs0->images[i], paths[i]);
    fs_destroy(fs0);

    // 2) Cargar FS desde disco
    FSImage *fs1 = fs_load(paths, 2, pwd);
    assert(fs1);
    // SB cargado coincide
    assert(fs1->sb->magic        == 0x42574653);
    assert(fs1->sb->segment_count == 2);
    // Bitmap recuperado coincide
    assert(fs_is_allocated(fs1, a) == 1);
    assert(fs_is_allocated(fs1, b) == 1);

    fs_destroy(fs1);
    printf("✅ Tests de fs_load pasaron exitosamente.\n");
    return 0;
}
