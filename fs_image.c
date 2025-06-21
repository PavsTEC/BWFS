#include "fs_image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

FSImage *fs_create(int width, int height, int block_size) {
    FSImage *fs = calloc(1, sizeof(FSImage));
    if (!fs) return NULL;
    fs->img = pbm_create(width, height);
    if (!fs->img) { free(fs); return NULL; }

    // Calcular bits totales disponibles
    size_t total_bits = (size_t)width * height;
    
    // Calcular bits necesarios para metadatos fijos
    size_t s_bits = sizeof(Superblock) * 8;  // Superbloque
    size_t d_bits = BWFS_MAX_FILES * sizeof(DirEntry) * 8;  // Directorio
    
    // Calcular bloques disponibles resolviendo la ecuación:
    // total_bits = s_bits + d_bits + block_count + (block_count * block_size)
    // => block_count * (block_size + 1) = total_bits - s_bits - d_bits
    if (total_bits <= s_bits + d_bits) {
        pbm_free(fs->img);
        free(fs);
        return NULL;
    }
    
    int block_count = (total_bits - s_bits - d_bits) / (block_size + 1);
    if (block_count <= 0) {
        pbm_free(fs->img);
        free(fs);
        return NULL;
    }

    // Inicializar superbloque con valores básicos
    sb_init(&fs->sb, width, height, block_size, block_count,
            BWFS_MAX_FILES, BWFS_MAX_BLOCKS_PER_FILE);
    
    // Establecer offsets precisos
    fs->sb.bitmap_offset = s_bits;
    fs->sb.dir_offset = s_bits + block_count; // bitmap = 1 bit por bloque
    fs->sb.data_offset = s_bits + block_count + d_bits;
    
    // Recalcular checksum con offsets actualizados
    fs->sb.checksum = sb_checksum(&fs->sb);

    // Inicializar bitmap y directorio
    bm_init(&fs->bm, fs->img, fs->sb.bitmap_offset, block_count);
    dir_init(&fs->dir);

    // Guardar superbloque actualizado
    sb_save(&fs->sb, fs->img);
    
    return fs;
}

FSImage *fs_load(const char *path) {
    FSImage *fs = calloc(1, sizeof(FSImage));
    if (!fs) return NULL;
    fs->img = pbm_load(path);
    if (!fs->img) { free(fs); return NULL; }

    if (sb_load(&fs->sb, fs->img) != 0) { pbm_free(fs->img); free(fs); return NULL; }
    bm_init(&fs->bm, fs->img, fs->sb.bitmap_offset, fs->sb.block_count);

    // Lectura optimizada del directorio
    uint8_t dir_bytes[sizeof(Directory)] = {0};
    int dir_start = fs->sb.dir_offset;
    for (size_t i = 0; i < sizeof(Directory) * 8; ++i) {
        int x = (dir_start + i) % fs->img->width;
        int y = (dir_start + i) / fs->img->width;
        int bit = pbm_get_pixel(fs->img, x, y);
        if (bit < 0) { pbm_free(fs->img); free(fs); return NULL; }
        dir_bytes[i / 8] |= (bit << (7 - (i % 8)));
    }
    dir_deserialize(&fs->dir, dir_bytes);

    return fs;
}

int fs_save(FSImage *fs, const char *path) {
    sb_save(&fs->sb, fs->img);
    
    uint8_t dir_bytes[sizeof(Directory)] = {0};
    dir_serialize(&fs->dir, dir_bytes);
    int dir_start = fs->sb.dir_offset;
    for (size_t i = 0; i < sizeof(Directory) * 8; ++i) {
        int x = (dir_start + i) % fs->img->width;
        int y = (dir_start + i) / fs->img->width;
        int bit = (dir_bytes[i / 8] >> (7 - (i % 8))) & 1;
        pbm_set_pixel(fs->img, x, y, bit);
    }
    
    return pbm_save(fs->img, path);
}

void fs_destroy(FSImage *fs) {
    if (fs) {
        if (fs->img) pbm_free(fs->img);
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

ssize_t fs_write_file(FSImage *fs, const char *name, const void *buf, size_t count) {
    int idx = dir_find(&fs->dir, name);
    if (idx < 0) return -1;
    DirEntry *e = dir_entry_mut(&fs->dir, idx);

    int block_bytes = fs->sb.block_size / 8;
    int blocks_needed = (count + block_bytes - 1) / block_bytes;
    if (blocks_needed > BWFS_MAX_BLOCKS_PER_FILE) return -2;

    // Rollback en caso de error
    for (uint32_t i = 0; i < e->block_count; ++i)
        bm_free(&fs->bm, e->blocks[i]);
    e->block_count = 0;
    e->size = count;

    const uint8_t *data = buf;
    size_t left = count;
    int allocated_blocks[BWFS_MAX_BLOCKS_PER_FILE];
    int alloc_count = 0;
    
    for (int i = 0; i < blocks_needed; ++i) {
        int b = bm_alloc_first(&fs->bm);
        if (b < 0) {
            // Rollback de bloques asignados
            for (int j = 0; j < alloc_count; j++)
                bm_free(&fs->bm, allocated_blocks[j]);
            return -3;
        }
        allocated_blocks[alloc_count++] = b;
        e->blocks[i] = b;
        e->block_count++;
        
        int block_start = fs->sb.data_offset + b * fs->sb.block_size;
        size_t to_write = left < (size_t)block_bytes ? left : (size_t)block_bytes;
        for (size_t j = 0; j < to_write * 8; ++j) {
            int x = (block_start + j) % fs->img->width;
            int y = (block_start + j) / fs->img->width;
            int bit = (data[j/8] >> (7 - (j % 8))) & 1;
            pbm_set_pixel(fs->img, x, y, bit);
        }
        left -= to_write;
        data += to_write;
    }
    
    // Checksum de datos
    uint32_t sum = 0;
    for (size_t i = 0; i < count; ++i)
        sum ^= ((const uint8_t*)buf)[i];
    e->checksum = sum;
    return count;
}

ssize_t fs_read_file(FSImage *fs, const char *name, void *buf, size_t count) {
    int idx = dir_find(&fs->dir, name);
    if (idx < 0) {
        printf("File not found: %s\n", name);
        return -1;
    }
    DirEntry *e = dir_entry_mut(&fs->dir, idx);

    size_t to_read = count < e->size ? count : e->size;
    if (to_read == 0) return 0;  // Archivo vacío
    
    uint8_t *data = buf;
    memset(buf, 0, count); // Inicializar buffer
    
    size_t pos = 0;
    int block_bytes = fs->sb.block_size / 8;
    
    for (uint32_t i = 0; i < e->block_count && pos < to_read; ++i) {
        int b = e->blocks[i];
        int block_start = fs->sb.data_offset + b * fs->sb.block_size;
        size_t read_now = (to_read - pos) < (size_t)block_bytes ? 
                         (to_read - pos) : (size_t)block_bytes;
                         
        for (size_t j = 0; j < read_now * 8; ++j) {
            int x = (block_start + j) % fs->img->width;
            int y = (block_start + j) / fs->img->width;
            int bit = pbm_get_pixel(fs->img, x, y);
            if (bit < 0) {
                printf("Error reading bit at (%d,%d)\n", x, y);
                return -2;
            }
            if (bit) {
                data[pos + j/8] |= (1 << (7 - (j % 8)));
            }
        }
        pos += read_now;
    }
    
    // Verificar checksum
    uint32_t sum = 0;
    for (size_t i = 0; i < to_read; ++i) {
        sum ^= data[i];
    }
    
    if (sum != e->checksum) {
        printf("Checksum failed for '%s': expected %u, got %u\n", 
               name, e->checksum, sum);
        return -3;
    }
    return (ssize_t)to_read;
}


int fs_check_integrity(FSImage *fs) {
    // 1. Verificar superbloque
    uint32_t sb_sum = sb_checksum(&fs->sb);
    if (sb_sum != fs->sb.checksum) {
        printf("Superblock checksum failed: expected %u, got %u\n",
               fs->sb.checksum, sb_sum);
        return -1;
    }

    // 2. Verificar directorio
    uint32_t dir_sum = dir_checksum(&fs->dir);
    if (dir_sum == 0 || dir_sum == 0xCAFEBABE) {
        printf("Directory checksum invalid: %u\n", dir_sum);
        return -2;
    }

    // 3. Verificar bitmap (consistencia global)
    uint32_t allocated_blocks = 0;
    for (int i = 0; i < fs->sb.block_count; i++) {
        if (bm_is_allocated(&fs->bm, i)) allocated_blocks++;
    }
    
    // 4. Verificar archivos individualmente
    for (int i = 0; i < BWFS_MAX_FILES; ++i) {
        if (fs->dir.entries[i].used) {
            // Verificar que los bloques asignados son válidos
            for (uint32_t j = 0; j < fs->dir.entries[i].block_count; j++) {
                int block_idx = fs->dir.entries[i].blocks[j];
                if (block_idx < 0 || (uint32_t)block_idx >= fs->sb.block_count) {
                    printf("Invalid block %d in file '%s'\n", 
                           block_idx, fs->dir.entries[i].name);
                    return -10 - i;
                }
                if (bm_is_allocated(&fs->bm, block_idx) != 1) {
                    printf("Block %d not allocated for file '%s'\n", 
                           block_idx, fs->dir.entries[i].name);
                    return -20 - i;
                }
            }
            
            // Leer el archivo
            uint8_t *tmp = malloc(fs->dir.entries[i].size);
            if (!tmp) return -30;
            
            ssize_t read = fs_read_file(fs, fs->dir.entries[i].name, 
                                       tmp, fs->dir.entries[i].size);
            if (read < 0) {
                printf("Read failed for '%s': %zd\n", 
                      fs->dir.entries[i].name, read);
                free(tmp);
                return -40 - i;
            }
            
            // Calcular checksum
            uint32_t sum = 0;
            for (size_t j = 0; j < fs->dir.entries[i].size; j++) {
                sum ^= tmp[j];
            }
            free(tmp);
            
            if (sum != fs->dir.entries[i].checksum) {
                printf("Checksum mismatch for '%s': expected %u, got %u\n",
                      fs->dir.entries[i].name, fs->dir.entries[i].checksum, sum);
                return -50 - i;
            }
        }
    }
    
    // 5. Verificar que no hay bloques asignados de más
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