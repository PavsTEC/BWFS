// fsck.bwfs.c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "fs_image.h"

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <bwfs_folder>\n", prog);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }
    const char *folder = argv[1];

    // Cargar el sistema de archivos
    FSImage *fs = fs_load(folder);
    if (!fs) {
        fprintf(stderr, "Error: no se pudo cargar BWFS desde '%s'\n", folder);
        return 1;
    }

    // Verificar la integridad
    int rc = fs_check_integrity(fs);
    if (rc == 0) {
        printf("BWFS consistente.\n");
        fs_destroy(fs);
        return 0;
    } else {
        printf("Inconsistencia detectada en BWFS (código %d).\n", rc);
        fs_destroy(fs);
        return 1;
    }
}
