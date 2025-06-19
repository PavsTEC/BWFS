#include <assert.h>
#include <stdio.h>
#include "pbm_manager.h"
#include <string.h>
#include "directory.h"

int main() {
    // 1) Crear imagen y inicializar directorio en bloque 20 con 100 entradas
    PBMImage *img = pbm_create(1024, 1024);
    assert(img);
    dir_init(img, /*dir_region_offset=*/20, /*max_entries=*/100);

    // 2) Lookup antes de crear → no existe
    assert(dir_lookup("foo") == -1);

    // 3) Agregar entrada
    assert(dir_add("foo", 5) == 0);

    // 4) Lookup tras agregar
    assert(dir_lookup("foo") == 5);

    // 5) Remover entrada
    assert(dir_remove("foo") == 0);
    assert(dir_lookup("foo") == -1);

    // 6) Re-agregar
    assert(dir_add("bar", 7) == 0);
    assert(dir_lookup("bar") == 7);

    // 7) Listar
    char names[10][DIR_NAME_MAX];
    uint32_t inodes[10];
    int n = dir_list(names, inodes, 10);
    assert(n == 1);
    assert(strncmp(names[0], "bar", DIR_NAME_MAX) == 0);
    assert(inodes[0] == 7);

    pbm_free(img);
    printf("✅ Tests de directory pasaron exitosamente.\n");
    return 0;
}
