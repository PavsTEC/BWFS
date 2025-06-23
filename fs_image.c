#include "fs_image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>


FSImage *fs_create(int width, int height, int block_size) {
    FSImage *fs = calloc(1, sizeof(FSImage));
    if (!fs) return NULL;

    // Crear la primera imagen
    PBMImage *first_img = pbm_create(width, height);
    if (!first_img) { free(fs); return NULL; }

    // Inicializar el sistema de archivos con la primera imagen
    fs->images = malloc(sizeof(PBMImage *));
    fs->images[0] = first_img;
    fs->image_count = 1;

    // Calcular bits totales disponibles en la primera imagen
    size_t total_bits = (size_t)width * height;

    // Calcular bits necesarios para metadatos fijos
    size_t s_bits = sizeof(Superblock) * 8;  // Superbloque
    size_t d_bits = BWFS_MAX_FILES * sizeof(DirEntry) * 8;  // Directorio

    // Calcular bloques disponibles resolviendo la ecuación:
    // total_bits = s_bits + d_bits + block_count + (block_count * block_size)
    // => block_count * (block_size + 1) = total_bits - s_bits - d_bits
    if (total_bits <= s_bits + d_bits) {
        pbm_free(first_img);
        free(fs->images);
        free(fs);
        return NULL;
    }

    int block_count = (total_bits - s_bits - d_bits) / (block_size + 1);
    if (block_count <= 0) {
        pbm_free(first_img);
        free(fs->images);
        free(fs);
        return NULL;
    }

    // Inicializar superbloque con valores básicos
    sb_init(&fs->sb, width, height, block_size, block_count,
            BWFS_MAX_FILES, BWFS_MAX_BLOCKS_PER_FILE);
    fs->sb.signature = BWFS_SIGNATURE;  // Establecer la firma

    // Establecer offsets precisos
    fs->sb.bitmap_offset = s_bits;
    fs->sb.dir_offset = s_bits + block_count; // bitmap = 1 bit por bloque
    fs->sb.data_offset = s_bits + block_count + d_bits;

    // Recalcular checksum con offsets actualizados
    fs->sb.checksum = sb_checksum(&fs->sb);

    // Inicializar bitmap y directorio
    bm_init(&fs->bm, first_img, fs->sb.bitmap_offset, block_count);
    dir_init(&fs->dir);

    // Guardar superbloque actualizado
    sb_save(&fs->sb, first_img);

    return fs;
}

void fs_add_image(FSImage *fs, int width, int height) {
    // Crear una nueva imagen
    PBMImage *new_img = pbm_create(width, height);
    if (!new_img) return;

    // Añadir la nueva imagen a la lista
    fs->images = realloc(fs->images, (fs->image_count + 1) * sizeof(PBMImage *));
    fs->images[fs->image_count++] = new_img;

    // Actualizar el superbloque y demás estructuras
    // (Por simplicidad, se asume que el superbloque se actualiza en fs_save)
}

FSImage *fs_load(const char *folder) {
    // 1) Reserva y limpia FSImage
    FSImage *fs = calloc(1, sizeof(*fs));
    if (!fs) return NULL;

    // 2) Carga la primera imagen (image_0.pbm)
    char path[1024];
    snprintf(path, sizeof(path), "%s/image_0.pbm", folder);
    PBMImage *img0 = pbm_load(path);
    if (!img0) {
        free(fs);
        return NULL;
    }

    // 3) Inicializa el array de imágenes
    fs->images      = malloc(sizeof(*fs->images));
    fs->images[0]   = img0;
    fs->image_count = 1;

    // 4) Carga superbloque desde image_0
    if (sb_load(&fs->sb, img0) < 0) {
        // error
    }

    // 5) Deserializa el directorio leyendo bits de image_0
    size_t bits_len  = sizeof(Directory) * 8;
    uint8_t *dir_bytes = calloc(sizeof(Directory), 1);
    for (size_t i = 0; i < bits_len; i++) {
        size_t bit_idx = fs->sb.dir_offset + i;
        int x = bit_idx % img0->width;
        int y = bit_idx / img0->width;
        int v = pbm_get_pixel(img0, x, y);
        dir_bytes[i/8] |= (uint8_t)((v & 1) << (7 - (i % 8)));
    }
    dir_deserialize(&fs->dir, dir_bytes);
    free(dir_bytes);

    // 6) Carga imágenes adicionales (image_1.pbm, image_2.pbm, …)
    for (int i = 1; ; i++) {
        snprintf(path, sizeof(path), "%s/image_%d.pbm", folder, i);
        PBMImage *img = pbm_load(path);
        if (!img) break;
        fs->images = realloc(fs->images,
                             sizeof(*fs->images) * (fs->image_count + 1));
        fs->images[fs->image_count++] = img;
    }

    // 7) Inicializa el bitmap sobre la última imagen cargada
    PBMImage *last = fs->images[fs->image_count - 1];
    bm_init(&fs->bm,
            last,
            fs->sb.bitmap_offset,
            fs->sb.block_count);

    return fs;
}

void fs_update_checksums(FSImage *fs) {
    // Actualizar checksum del directorio
    fs->sb.dir_checksum = dir_checksum(&fs->dir);
    
    // Actualizar checksum del superbloque
    fs->sb.checksum = 0;
    fs->sb.checksum = sb_checksum(&fs->sb);
}

int fs_save(FSImage *fs, const char *folder_path) {
    // Actualizar checksums antes de guardar
    fs_update_checksums(fs);
    
    // Guardar superbloque primero
    sb_save(&fs->sb, fs->images[0]);

    // Serializar y guardar el directorio en la primera imagen
    uint8_t dir_bytes[sizeof(Directory)] = {0};
    dir_serialize(&fs->dir, dir_bytes);
    int dir_start = fs->sb.dir_offset;
    for (size_t i = 0; i < sizeof(Directory) * 8; ++i) {
        int x = (dir_start + i) % fs->images[0]->width;
        int y = (dir_start + i) / fs->images[0]->width;
        int bit = (dir_bytes[i / 8] >> (7 - (i % 8))) & 1;
        pbm_set_pixel(fs->images[0], x, y, bit);
    }

    // Guardar todas las imágenes
    for (int i = 0; i < fs->image_count; ++i) {
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s/image_%d.pbm", folder_path, i);
        printf("Guardando imagen %d en %s\n", i, file_path);
        if (pbm_save(fs->images[i], file_path) != 0) {
            printf("Error al guardar la imagen %d\n", i);
            return -1;
        }
    }

    return 0;
}

void fs_destroy(FSImage *fs) {
    if (fs) {
        for (int i = 0; i < fs->image_count; ++i) {
            if (fs->images[i]) pbm_free(fs->images[i]);
        }
        free(fs->images);
        free(fs);
    }
}

int fs_create_file(FSImage *fs, const char *name) {
    return dir_create(&fs->dir, name);
}

int fs_remove_file(FSImage *fs, const char *name) {
    int idx = dir_find(&fs->dir, name);
    if (idx < 0) return -1;

    DirEntry *e = dir_entry_mut(&fs->dir, idx);
    for (uint32_t i = 0; i < e->block_count; ++i)
        bm_free(&fs->bm, e->blocks[i]);

    return dir_remove(&fs->dir, name);
}

ssize_t fs_write_file(FSImage *fs,
                      const char *filename,
                      const void *buf,
                      size_t size)
{
    const uint8_t *buffer = (const uint8_t *)buf;

    // 1) Busca la entrada en el directorio
    int idx = dir_find(&fs->dir, filename);
    if (idx < 0) return -1;
    DirEntry *e = dir_entry_mut(&fs->dir, idx);

    // 2) Comprueba tamaño máximo permitido
    size_t max_bytes = fs->sb.max_blocks_per_file * (fs->sb.block_size / 8);
    if (size > max_bytes) return -1;

    // 3) Libera bloques previos del archivo (si existían)
    for (uint32_t i = 0; i < e->block_count; i++) {
        int g = e->blocks[i];
        int img = g / fs->sb.block_count;
        uint32_t loc = g % fs->sb.block_count;
        BlockManager bm_tmp;
        bm_init(&bm_tmp,
                fs->images[img],
                fs->sb.bitmap_offset,
                fs->sb.block_count);
        bm_free(&bm_tmp, loc);
    }
    e->block_count = 0;

    // 4) Escribe los datos, bloque a bloque, expandiendo imágenes si es necesario
    size_t written     = 0;
    size_t block_bytes = fs->sb.block_size / 8;

    while (written < size) {
        size_t to_write = (size - written < block_bytes)
                            ? size - written
                            : block_bytes;

        // 4.1) Intenta asignar en la imagen actual
        int loc = bm_alloc_first(&fs->bm);
        if (loc < 0) {
            // Crea y usa una nueva imagen
            PBMImage *new_img = pbm_create(fs->sb.width, fs->sb.height);
            fs->images = realloc(fs->images,
                                 sizeof(*fs->images) * (fs->image_count + 1));
            fs->images[fs->image_count] = new_img;
            fs->image_count++;

            bm_init(&fs->bm,
                    new_img,
                    fs->sb.bitmap_offset,
                    fs->sb.block_count);
            loc = bm_alloc_first(&fs->bm);
            if (loc < 0) {
                // Ya no queda espacio incluso tras expandir
                return -3;
            }
        }

        // 4.2) Guarda el índice global de bloque
        int img_idx      = fs->image_count - 1;
        int global_block = img_idx * (int)fs->sb.block_count + loc;
        e->blocks[e->block_count++] = global_block;

        // 4.3) Pintar bits en la PBM correspondiente
        PBMImage *img = fs->images[img_idx];
        size_t data_off = fs->sb.data_offset;
        for (size_t bit = 0; bit < to_write * 8; bit++) {
            size_t bit_idx = data_off
                           + (size_t)loc * fs->sb.block_size
                           + bit;
            int x = bit_idx % img->width;
            int y = bit_idx / img->width;
            size_t byte_i = written + (bit / 8);
            int bit_i = 7 - (bit % 8);
            int val = (buffer[byte_i] >> bit_i) & 1;
            pbm_set_pixel(img, x, y, val);
        }

        written += to_write;
    }

    // 5) Actualiza tamaño y checksum del archivo
    e->size = size;
    uint32_t file_sum = 0;
    for (size_t i = 0; i < size; i++) {
        file_sum ^= buffer[i];
    }
    e->checksum = file_sum;

    // 6) Actualiza checksum del directorio y superbloque
    fs_update_checksums(fs);

    return (ssize_t)written;
}


ssize_t fs_read_file(FSImage *fs,
                     const char *filename,
                     void *buf,
                     size_t size)
{
    // 1) Busca la entrada
    int idx = dir_find(&fs->dir, filename);
    if (idx < 0) return -1;
    DirEntry *e = dir_entry_mut(&fs->dir, idx);

    // 2) No leer más de lo que pide el usuario
    size_t to_read_total = (size < e->size) ? size : e->size;
    uint8_t *buffer = (uint8_t *)buf;
    memset(buffer, 0, to_read_total);

    // 3) Parámetros
    size_t block_bytes = fs->sb.block_size / 8;
    size_t read_bytes  = 0;

    // 4) Recorre cada bloque asignado
    for (uint32_t i = 0; i < e->block_count && read_bytes < to_read_total; i++) {
        int g = e->blocks[i];
        int img_idx = g / fs->sb.block_count;
        uint32_t loc = g % fs->sb.block_count;

        // Validación simple
        if (img_idx < 0 || img_idx >= fs->image_count) {
            printf("Invalid image index %d in file '%s'\n",
                   img_idx, e->name);
            return -2;
        }

        PBMImage *img = fs->images[img_idx];
        size_t data_off = fs->sb.data_offset;
        size_t this_read = (to_read_total - read_bytes < block_bytes)
                             ? (to_read_total - read_bytes)
                             : block_bytes;

        // 5) Bit a bit desde la imagen PBM
        for (size_t bit = 0; bit < this_read * 8; bit++) {
            size_t bit_idx = data_off
                           + (size_t)loc * fs->sb.block_size
                           + bit;
            int x = bit_idx % img->width;
            int y = bit_idx / img->width;
            int v = pbm_get_pixel(img, x, y);
            if (v < 0) {
                printf("Error reading bit at (%d,%d)\n", x, y);
                return -2;
            }
            size_t byte_i = read_bytes + (bit / 8);
            int bit_i = 7 - (bit % 8);
            buffer[byte_i] |= (v << bit_i);
        }

        read_bytes += this_read;
    }

    return (ssize_t)read_bytes;
}

int fs_check_integrity(FSImage *fs) {
    // 1. Verificar superbloque
    uint32_t stored_sb_checksum = fs->sb.checksum;
    Superblock tmp_sb = fs->sb;
    tmp_sb.checksum = 0;
    uint32_t computed_sb_checksum = sb_checksum(&tmp_sb);
    if (computed_sb_checksum != stored_sb_checksum) {
        printf("Superblock checksum failed: stored=%u, computed=%u\n",
               stored_sb_checksum, computed_sb_checksum);
        return -1;
    }

    // 2. Verificar checksum del directorio
    uint32_t computed_dir_sum = dir_checksum(&fs->dir);
    if (computed_dir_sum != fs->sb.dir_checksum) {
        printf("Directory checksum mismatch: stored=%u, computed=%u\n",
               fs->sb.dir_checksum, computed_dir_sum);
        return -2;
    }

    // 3. Contar bloques asignados en todas las imágenes
    uint32_t allocated_blocks = 0;
    for (int img = 0; img < fs->image_count; img++) {
        BlockManager bm_tmp;
        bm_init(&bm_tmp,
                fs->images[img],
                fs->sb.bitmap_offset,
                fs->sb.block_count);
        for (uint32_t b = 0; b < fs->sb.block_count; b++) {
            if (bm_is_allocated(&bm_tmp, b) == 1) {
                allocated_blocks++;
            }
        }
    }

    // 4. Verificar cada archivo
    int total_blocks = fs->image_count * fs->sb.block_count;
    for (int i = 0; i < BWFS_MAX_FILES; i++) {
        DirEntry *e = &fs->dir.entries[i];
        if (!e->used) continue;

        // 4a) Verificar que todos los bloques globales son válidos y estén marcados
        for (uint32_t j = 0; j < e->block_count; j++) {
            int g = e->blocks[j];
            if (g < 0 || g >= total_blocks) {
                printf("Invalid global block %d in file '%s'\n",
                       g, e->name);
                return -10 - i;
            }
            int img = g / fs->sb.block_count;
            int loc = g % fs->sb.block_count;
            BlockManager bm_tmp;
            bm_init(&bm_tmp,
                    fs->images[img],
                    fs->sb.bitmap_offset,
                    fs->sb.block_count);
            if (bm_is_allocated(&bm_tmp, loc) != 1) {
                printf("Block %d not allocated for file '%s'\n",
                       g, e->name);
                return -20 - i;
            }
        }

        // 4b) Leer y verificar checksum del contenido
        uint8_t *buf = malloc(e->size);
        if (!buf) return -30;
        ssize_t rd = fs_read_file(fs, e->name, buf, e->size);
        if (rd < 0) {
            printf("Read failed for '%s': %zd\n", e->name, rd);
            free(buf);
            return -40 - i;
        }
        uint32_t sum = 0;
        for (size_t k = 0; k < e->size; k++) {
            sum ^= buf[k];
        }
        free(buf);
        if (sum != e->checksum) {
            printf("Checksum mismatch for '%s': expected %u, got %u\n",
                   e->name, e->checksum, sum);
            return -50 - i;
        }
    }

    // 5. Verificar que el número de bloques marcados coincide con la suma de e->block_count
    uint32_t files_blocks = 0;
    for (int i = 0; i < BWFS_MAX_FILES; i++) {
        if (fs->dir.entries[i].used) {
            files_blocks += fs->dir.entries[i].block_count;
        }
    }
    if (allocated_blocks != files_blocks) {
        printf("Block count mismatch: bitmap=%u, files=%u\n",
               allocated_blocks, files_blocks);
        return -3;
    }

    return 0;
}