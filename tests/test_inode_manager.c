#include <assert.h>
#include <stdio.h>
#include "pbm_manager.h"
#include "inode_manager.h"

int main() {
    // 1) Crear imagen de prueba
    PBMImage *img = pbm_create(1024, 1024);
    assert(img);

    // 2) Inicializar módulo de i-nodos con la imagen
    inode_init(img, /*offset=*/10, /*max_inodes=*/100);

    // 3) Reservar un i-nodo
    int idx = inode_alloc();
    assert(idx >= 0 && idx < 100);

    // 4) Guardar metadatos en el i-nodo
    Inode node = { .size = 12345 };
    node.direct[0] = 5;
    assert(inode_save(idx, &node) == 0);

    // 5) Cargar y verificar
    Inode loaded;
    assert(inode_load(idx, &loaded) == 0);
    assert(loaded.size == node.size);
    assert(loaded.direct[0] == node.direct[0]);

    // 6) Liberar i-nodo y volver a reservar debe regresar el mismo índice
    assert(inode_free(idx) == 0);
    assert(inode_alloc() == idx);

    pbm_free(img);
    printf("✅ Tests de inode_manager pasaron exitosamente.\n");
    return 0;
}
