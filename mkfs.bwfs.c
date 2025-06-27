// mkfs.bwfs.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include "fs_image.h"

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [-w width] [-h height] [-b block_bits] folder\n"
        "  -w width        Image width in pixels (1-1000). Default 1000.\n"
        "  -h height       Image height in pixels (1-1000). Default 1000.\n"
        "  -b block_bits   Block size in bits (≤ width*height). Default 1024.\n",
        prog);
    exit(1);
}

int main(int argc, char *argv[]) {
    int width      = 1000;
    int height     = 1000;
    size_t bbits   = 1024;
    int opt;

    while ((opt = getopt(argc, argv, "w:h:b:")) != -1) {
        switch (opt) {
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 'b':
            bbits = (size_t)atoi(optarg);
            break;
        default:
            usage(argv[0]);
        }
    }
    if (optind >= argc) usage(argv[0]);
    const char *folder = argv[optind];

    if (width < 1 || width > 1000 ||
        height < 1 || height > 1000) {
        fprintf(stderr, "Error: width and height must be between 1 and 1000\n");
        return 1;
    }
    if (bbits < 1 || bbits > (size_t)width * height) {
        fprintf(stderr, "Error: block_bits must be ≥1 and ≤ width*height (%d×%d=%d)\n",
                width, height, width*height);
        return 1;
    }

    // Crear directorio
    if (mkdir(folder, 0755) < 0) {
        perror("mkdir");
        return 1;
    }

    // Crear y guardar el FS
    FSImage *fs = fs_create(width, height, bbits);
    if (!fs) {
        fprintf(stderr, "Error: no se pudo crear el BWFS\n");
        return 1;
    }
    if (fs_save(fs, folder) < 0) {
        fprintf(stderr, "Error: no se pudo guardar el BWFS en '%s'\n", folder);
        fs_destroy(fs);
        return 1;
    }
    fs_destroy(fs);

    printf("BWFS creado en '%s' (ancho=%d, alto=%d, block_bits=%zu)\n",
           folder, width, height, bbits);
    return 0;
}
