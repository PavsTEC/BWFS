#include "directory.h"
#include "superblock.h"   // para SUPERBLOCK_SIZE
#include <string.h>
#include <stdlib.h>

static PBMImage *dir_image   = NULL;
static int       dir_offset  = 0;
static int       dir_count   = 0;
static const int entry_size  = sizeof(DirEntry);

/** Inicializa el módulo */
void dir_init(PBMImage *image, int dir_region_offset, int max_entries) {
    dir_image  = image;
    dir_offset = dir_region_offset;
    dir_count  = max_entries;
}

/** Devuelve el píxel de inicio para la entrada idx */
static int entry_pixel_start(int idx) {
    return dir_offset * SUPERBLOCK_SIZE + idx * entry_size;
}

/** Lee la entrada idx desde la imagen */
static void read_entry(int idx, DirEntry *out) {
    int start = entry_pixel_start(idx);
    for (int i = 0; i < entry_size; i++) {
        ((uint8_t*)out)[i] = dir_image->pixels[start + i];
    }
}

/** Escribe la entrada idx en la imagen */
static void write_entry(int idx, const DirEntry *in) {
    int start = entry_pixel_start(idx);
    for (int i = 0; i < entry_size; i++) {
        dir_image->pixels[start + i] = ((const uint8_t*)in)[i];
    }
}

int dir_add(const char *name, uint32_t inode) {
    if (!dir_image || !name || inode == 0) return -1;
    DirEntry e;
    // Buscar primera libre (inode==0)
    for (int i = 0; i < dir_count; i++) {
        read_entry(i, &e);
        if (e.inode == 0) {
            // Crear nueva entrada
            memset(&e, 0, entry_size);
            e.inode = inode;
            // Copiar nombre (hasta DIR_NAME_MAX-1)
            strncpy(e.name, name, DIR_NAME_MAX-1);
            write_entry(i, &e);
            return 0;
        }
    }
    return -1;
}

int dir_remove(const char *name) {
    if (!dir_image || !name) return -1;
    DirEntry e;
    for (int i = 0; i < dir_count; i++) {
        read_entry(i, &e);
        if (e.inode != 0 && strncmp(e.name, name, DIR_NAME_MAX) == 0) {
            // Borrar: poner a cero
            DirEntry zero = {0};
            write_entry(i, &zero);
            return 0;
        }
    }
    return -1;
}

int dir_lookup(const char *name) {
    if (!dir_image || !name) return -1;
    DirEntry e;
    for (int i = 0; i < dir_count; i++) {
        read_entry(i, &e);
        if (e.inode != 0 && strncmp(e.name, name, DIR_NAME_MAX) == 0) {
            return e.inode;
        }
    }
    return -1;
}

int dir_list(char names[][DIR_NAME_MAX], uint32_t inodes[], int max_count) {
    if (!dir_image || !names || !inodes) return 0;
    DirEntry e;
    int cnt = 0;
    for (int i = 0; i < dir_count && cnt < max_count; i++) {
        read_entry(i, &e);
        if (e.inode != 0) {
            strncpy(names[cnt], e.name, DIR_NAME_MAX);
            inodes[cnt] = e.inode;
            cnt++;
        }
    }
    return cnt;
}
