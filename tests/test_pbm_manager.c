#include <assert.h>
#include <stdio.h>
#include "pbm_manager.h"

int main() {
    const char *testfile = "test_output.pbm";

    // 1) Crear imagen 8x8 y verificar dimensiones
    PBMImage *img = pbm_create(8, 8);
    assert(img != NULL);
    assert(img->width == 8 && img->height == 8);

    // 2) Probar pbm_set_pixel y pbm_get_pixel
    assert(pbm_set_pixel(img, 0, 0, 1) == 0);
    assert(pbm_set_pixel(img, 7, 7, 1) == 0);
    assert(pbm_get_pixel(img, 0, 0) == 1);
    assert(pbm_get_pixel(img, 7, 7) == 1);
    // Coordenadas inválidas
    assert(pbm_get_pixel(img, -1, 0) == -1);
    assert(pbm_set_pixel(img, 8, 8, 1) == -1);

    // 3) Guardar en disco y recargar
    assert(pbm_save(img, testfile) == 0);
    PBMImage *loaded = pbm_load(testfile);
    assert(loaded != NULL);
    assert(loaded->width == 8 && loaded->height == 8);
    assert(pbm_get_pixel(loaded, 0, 0) == 1);
    assert(pbm_get_pixel(loaded, 7, 7) == 1);

    // 4) Limpieza
    pbm_free(img);
    pbm_free(loaded);
    remove(testfile);

    printf("✅ Todos los tests de pbm_manager pasaron exitosamente.\n");
    return 0;
}
