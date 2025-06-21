#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pbm_manager.h"
#include "block_manager.h"
#include "superblock.h"
#include "directory.h"
#include "fs_image.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define TEST_WIDTH       1024
#define TEST_HEIGHT      1024
#define TEST_BLOCK_SIZE  1024    // en bits
#define TEST_PBM_FILE    "test.pbm"
#define MAX_FILE_SIZE   (BWFS_MAX_BLOCKS_PER_FILE * (TEST_BLOCK_SIZE / 8))

// Función para generar datos aleatorios
void generate_random_data(uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] = rand() % 256;
    }
}

// 1) pbm_manager
static void test_pbm_manager(void) {
    printf("\n=== test_pbm_manager ===\n");
    PBMImage *img = pbm_create(TEST_WIDTH, TEST_HEIGHT);
    assert(img);
    
    // Prueba de escritura/lectura de píxeles
    for (int i = 0; i < 100; i++) {
        int x = rand() % TEST_WIDTH;
        int y = rand() % TEST_HEIGHT;
        int value = rand() % 2;
        
        assert(pbm_set_pixel(img, x, y, value) == 0);
        assert(pbm_get_pixel(img, x, y) == value);
    }
    
    // Prueba de guardado y carga
    assert(pbm_save(img, TEST_PBM_FILE) == 0);
    PBMImage *img2 = pbm_load(TEST_PBM_FILE);
    assert(img2);
    
    // Verificar consistencia
    for (int i = 0; i < 100; i++) {
        int x = rand() % TEST_WIDTH;
        int y = rand() % TEST_HEIGHT;
        assert(pbm_get_pixel(img, x, y) == pbm_get_pixel(img2, x, y));
    }

    pbm_free(img);
    pbm_free(img2);
    remove(TEST_PBM_FILE);
    printf("✔ pbm_manager\n");
}

// 2) block_manager
static void test_block_manager(void) {
    printf("\n=== test_block_manager ===\n");
    PBMImage *img = pbm_create(TEST_WIDTH, TEST_HEIGHT);
    BlockManager bm;
    int total_blocks = (TEST_WIDTH * TEST_HEIGHT) / TEST_BLOCK_SIZE;
    bm_init(&bm, img, 0, total_blocks);

    // Prueba de asignación y liberación
    int allocated[100] = {0};
    for (int i = 0; i < 100; i++) {
        int block = bm_alloc_first(&bm);
        assert(block >= 0 && block < total_blocks);
        allocated[i] = block;
        assert(bm_is_allocated(&bm, block) == 1);
    }
    
    // Liberar bloques
    for (int i = 0; i < 100; i++) {
        assert(bm_free(&bm, allocated[i]) == 0);
        assert(bm_is_allocated(&bm, allocated[i]) == 0);
    }
    
    // Prueba de asignación exhaustiva
    int blocks[total_blocks];
    for (int i = 0; i < total_blocks; i++) {
        blocks[i] = bm_alloc_first(&bm);
        assert(blocks[i] == i); // Debe asignar en orden
    }
    
    // Intentar asignar cuando no hay espacio
    assert(bm_alloc_first(&bm) == -1);
    
    // Liberar todos
    for (int i = 0; i < total_blocks; i++) {
        assert(bm_free(&bm, i) == 0);
    }
    
    pbm_free(img);
    printf("✔ block_manager\n");
}

// 3) superblock
static void test_superblock(void) {
    printf("\n=== test_superblock ===\n");
    PBMImage *img = pbm_create(TEST_WIDTH, TEST_HEIGHT);
    assert(img);
    
    int block_count = 64;
    Superblock sb;
    sb_init(&sb, TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE, 
            block_count, BWFS_MAX_FILES, BWFS_MAX_BLOCKS_PER_FILE);
    
    // Verificar offsets
    size_t expected_bitmap = sizeof(Superblock) * 8;
    size_t expected_dir = expected_bitmap + block_count;
    size_t expected_data = expected_dir + BWFS_MAX_FILES * sizeof(DirEntry) * 8;
    
    assert(sb.bitmap_offset == expected_bitmap);
    assert(sb.dir_offset == expected_dir);
    assert(sb.data_offset == expected_data);
    
    // Guardar y cargar
    assert(sb_save(&sb, img) == 0);
    
    Superblock sb2;
    memset(&sb2, 0, sizeof(sb2));
    assert(sb_load(&sb2, img) == 0);
    
    // Verificar todos los campos
    assert(sb2.magic == sb.magic);
    assert(sb2.width == sb.width);
    assert(sb2.height == sb.height);
    assert(sb2.block_size == sb.block_size);
    assert(sb2.block_count == sb.block_count);
    assert(sb2.max_files == sb.max_files);
    assert(sb2.max_blocks_per_file == sb.max_blocks_per_file);
    assert(sb2.bitmap_offset == sb.bitmap_offset);
    assert(sb2.dir_offset == sb.dir_offset);
    assert(sb2.data_offset == sb.data_offset);
    assert(sb2.checksum == sb.checksum);
    
    // Prueba de checksum inválido
    sb2.checksum = 0;
    assert(sb_checksum(&sb2) != 0);
    
    pbm_free(img);
    printf("✔ superblock\n");
}

// 4) directory
static void test_directory(void) {
    printf("\n=== test_directory ===\n");
    Directory dir;
    dir_init(&dir);
    
    // Crear archivos
    char filename[32];
    for (int i = 0; i < BWFS_MAX_FILES; i++) {
        snprintf(filename, sizeof(filename), "file%d.txt", i);
        assert(dir_create(&dir, filename) == i);
    }
    
    // Intentar crear cuando está lleno
    assert(dir_create(&dir, "extra.txt") == -1);
    
    // Encontrar archivos
    for (int i = 0; i < BWFS_MAX_FILES; i++) {
        snprintf(filename, sizeof(filename), "file%d.txt", i);
        assert(dir_find(&dir, filename) == i);
    }
    
    // Eliminar archivos alternados
    for (int i = 0; i < BWFS_MAX_FILES; i += 2) {
        snprintf(filename, sizeof(filename), "file%d.txt", i);
        assert(dir_remove(&dir, filename) == 0);
        assert(dir_find(&dir, filename) == -1);
    }
    
    // Crear nuevos archivos en espacios liberados
    for (int i = 0; i < BWFS_MAX_FILES; i += 2) {
        snprintf(filename, sizeof(filename), "newfile%d.txt", i);
        assert(dir_create(&dir, filename) == i);
        assert(dir_find(&dir, filename) == i);
    }
    
    // Intentar eliminar archivo inexistente
    assert(dir_remove(&dir, "noexist.txt") == -1);
    
    printf("✔ directory\n");
}

// 5) Pruebas de archivos
static void test_file_operations(FSImage *fs) {
    printf("\n=== test_file_operations ===\n");
    
    // Crear archivo
    const char *filename = "testfile.bin";
    assert(fs_create_file(fs, filename) == 0);
    
    // Tamaños a probar
    size_t sizes[] = {1, 128, 256, 512, 1024, 2048, 4096, MAX_FILE_SIZE};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        size_t size = sizes[i];
        if (size > MAX_FILE_SIZE) continue;
        
        // Generar datos aleatorios
        uint8_t *w = malloc(size);
        uint8_t *r = calloc(1, size);
        assert(w && r);
        generate_random_data(w, size);
        
        // Escribir archivo
        ssize_t wrote = fs_write_file(fs, filename, w, size);
        printf("Tamaño %zu: escribió %zd bytes\n", size, wrote);
        assert(wrote == (ssize_t)size);
        
        // Leer archivo
        ssize_t readb = fs_read_file(fs, filename, r, size);
        printf("Tamaño %zu: leyó %zd bytes\n", size, readb);
        assert(readb == (ssize_t)size);
        
        // Verificar datos
        assert(memcmp(w, r, size) == 0);
        
        free(w);
        free(r);
    }
    
    // Prueba de sobreescritura
    size_t size1 = 1024;
    size_t size2 = 2048;
    uint8_t *w1 = malloc(size1);
    uint8_t *w2 = malloc(size2);
    uint8_t *r = calloc(1, size2);
    assert(w1 && w2 && r);
    
    generate_random_data(w1, size1);
    generate_random_data(w2, size2);
    
    // Escribir primera versión
    assert(fs_write_file(fs, filename, w1, size1) == (ssize_t)size1);
    assert(fs_read_file(fs, filename, r, size1) == (ssize_t)size1);
    assert(memcmp(w1, r, size1) == 0);
    
    // Escribir segunda versión (más grande)
    assert(fs_write_file(fs, filename, w2, size2) == (ssize_t)size2);
    assert(fs_read_file(fs, filename, r, size2) == (ssize_t)size2);
    assert(memcmp(w2, r, size2) == 0);
    
    // Escribir tercera versión (más pequeña)
    assert(fs_write_file(fs, filename, w1, size1) == (ssize_t)size1);
    assert(fs_read_file(fs, filename, r, size1) == (ssize_t)size1);
    assert(memcmp(w1, r, size1) == 0);
    
    free(w1);
    free(w2);
    free(r);
    
    // Eliminar archivo
    assert(fs_remove_file(fs, filename) == 0);
    printf("✔ test_file_operations\n");
}

// 6) Prueba de múltiples archivos
static void test_multiple_files(FSImage *fs) {
    printf("\n=== test_multiple_files ===\n");
    
    // Crear múltiples archivos
    char filename[32];
    for (int i = 0; i < 10; i++) {
        snprintf(filename, sizeof(filename), "multi%d.bin", i);
        int rc = fs_create_file(fs, filename);
        if (rc < 0) {  // Cambiado a < 0 para detectar errores
            printf("Error creando archivo '%s': %d\n", filename, rc);
            if (dir_find(&fs->dir, filename) >= 0) {
                printf("El archivo '%s' ya existe\n", filename);
            }
        }
        assert(rc >= 0);  // Verificar que es un índice válido
        
        size_t size = 128 * (i + 1);
        uint8_t *data = malloc(size);
        uint8_t *rdata = calloc(1, size);
        assert(data && rdata);
        generate_random_data(data, size);
        
        ssize_t wrote = fs_write_file(fs, filename, data, size);
        assert(wrote == (ssize_t)size);
        
        ssize_t readb = fs_read_file(fs, filename, rdata, size);
        assert(readb == (ssize_t)size);
        assert(memcmp(data, rdata, size) == 0);
        
        free(data);
        free(rdata);
    }
    
    // Leer todos los archivos nuevamente
    for (int i = 0; i < 10; i++) {
        snprintf(filename, sizeof(filename), "multi%d.bin", i);
        size_t size = 128 * (i + 1);
        uint8_t *rdata = calloc(1, size);
        assert(rdata);
        
        ssize_t readb = fs_read_file(fs, filename, rdata, size);
        assert(readb == (ssize_t)size);
        free(rdata);
    }
    
    // Eliminar algunos archivos
    for (int i = 0; i < 10; i += 2) {
        snprintf(filename, sizeof(filename), "multi%d.bin", i);
        assert(fs_remove_file(fs, filename) == 0);
    }
    
    // Crear nuevos archivos en su lugar
    for (int i = 0; i < 10; i += 2) {
        snprintf(filename, sizeof(filename), "new_multi%d.bin", i);
        int rc = fs_create_file(fs, filename);
        if (rc < 0) {
            printf("Error creando archivo '%s': %d\n", filename, rc);
        }
        assert(rc >= 0);  // Verificar que es un índice válido
        
        size_t size = 256 * (i + 1);
        uint8_t *data = malloc(size);
        assert(data);
        generate_random_data(data, size);
        
        ssize_t wrote = fs_write_file(fs, filename, data, size);
        assert(wrote == (ssize_t)size);
        free(data);
    }
    
    // Verificar integridad
    assert(fs_check_integrity(fs) == 0);
    printf("✔ test_multiple_files\n");
}

// 7) Prueba de límites
static void test_boundaries(FSImage *fs) {
    printf("\n=== test_boundaries ===\n");
    
    // Crear archivo con nombre largo
    char long_name[BWFS_FILENAME_MAXLEN + 10];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    // Nombre debería ser truncado
    int rc = fs_create_file(fs, long_name);
    if (rc < 0) {
        printf("Error creando archivo con nombre largo: %d\n", rc);
    }
    assert(rc >= 0);
    
    // Intentar crear archivo sin nombre (debe fallar)
    int empty_rc = fs_create_file(fs, "");
    printf("Crear archivo sin nombre: resultado %d (esperado -1)\n", empty_rc);
    assert(empty_rc == -1);
    
    // Crear archivo con tamaño exacto a un bloque
    const char *block_file = "block_file.bin";
    int block_rc = fs_create_file(fs, block_file);
    if (block_rc < 0) {
        printf("Error creando archivo '%s': %d\n", block_file, block_rc);
    }
    assert(block_rc >= 0);  // Índice válido en directorio
    
    size_t block_bytes = TEST_BLOCK_SIZE / 8;
    uint8_t *block_data = malloc(block_bytes);
    uint8_t *block_rdata = calloc(1, block_bytes);
    assert(block_data && block_rdata);
    generate_random_data(block_data, block_bytes);
    
    ssize_t wrote = fs_write_file(fs, block_file, block_data, block_bytes);
    assert(wrote == (ssize_t)block_bytes);
    
    ssize_t readb = fs_read_file(fs, block_file, block_rdata, block_bytes);
    assert(readb == (ssize_t)block_bytes);
    assert(memcmp(block_data, block_rdata, block_bytes) == 0);
    
    free(block_data);
    free(block_rdata);
    
    // Intentar escribir más allá del límite
    const char *full_file = "full_file.bin";
    int full_rc = fs_create_file(fs, full_file);
    if (full_rc < 0) {
        printf("Error creando archivo '%s': %d\n", full_file, full_rc);
    }
    assert(full_rc >= 0);  // Índice válido en directorio
    
    uint8_t *big_data = malloc(MAX_FILE_SIZE + 1);
    assert(big_data);
    generate_random_data(big_data, MAX_FILE_SIZE + 1);
    
    // Debería fallar
    ssize_t result = fs_write_file(fs, full_file, big_data, MAX_FILE_SIZE + 1);
    assert(result < 0);
    
    // Escribir tamaño máximo
    assert(fs_write_file(fs, full_file, big_data, MAX_FILE_SIZE) == (ssize_t)MAX_FILE_SIZE);
    
    free(big_data);
    
    // Leer más allá del tamaño del archivo
    uint8_t *big_buffer = malloc(MAX_FILE_SIZE + 1024);
    assert(big_buffer);
    ssize_t read = fs_read_file(fs, full_file, big_buffer, MAX_FILE_SIZE + 1024);
    assert(read == (ssize_t)MAX_FILE_SIZE);
    
    free(big_buffer);
    
    printf("✔ test_boundaries\n");
}

// 8) Prueba de persistencia
static void test_persistence(void) {
    printf("\n=== test_persistence ===\n");
    const char *filename = "persistent.bin";
    const char *fs_file = "persistent.pbm";
    
    // Crear sistema de archivos
    FSImage *fs = fs_create(TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE);
    assert(fs);
    
    // Crear y escribir archivo
    size_t size = 2048;
    uint8_t *data = malloc(size);
    uint8_t *rdata = calloc(1, size);
    assert(data && rdata);
    generate_random_data(data, size);
    
    assert(fs_create_file(fs, filename) == 0);
    assert(fs_write_file(fs, filename, data, size) == (ssize_t)size);
    assert(fs_save(fs, fs_file) == 0);
    fs_destroy(fs);
    
    // Cargar sistema de archivos
    fs = fs_load(fs_file);
    assert(fs);
    
    // Verificar integridad
    assert(fs_check_integrity(fs) == 0);
    
    // Leer archivo
    assert(fs_read_file(fs, filename, rdata, size) == (ssize_t)size);
    assert(memcmp(data, rdata, size) == 0);
    
    // Limpieza
    free(data);
    free(rdata);
    fs_destroy(fs);
    remove(fs_file);
    printf("✔ test_persistence\n");
}

// 9) Prueba de integridad (fsck)
static void test_fsck(void) {
    printf("\n=== test_fsck ===\n");
    FSImage *fs = fs_create(TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE);
    assert(fs);
    
    // Crear archivo de prueba
    const char *filename = "testfile.bin";
    int file_idx = fs_create_file(fs, filename);
    assert(file_idx >= 0);
    
    // Escribir datos conocidos
    uint8_t data[256];
    for (int i = 0; i < 256; i++) data[i] = i;
    assert(fs_write_file(fs, filename, data, 256) == 256);
    
    // Guardar y recargar
    assert(fs_save(fs, TEST_PBM_FILE) == 0);
    fs_destroy(fs);
    fs = fs_load(TEST_PBM_FILE);
    assert(fs);
    
    // === Prueba de corrupción de datos ===
    int idx = dir_find(&fs->dir, filename);
    assert(idx >= 0);
    DirEntry *e = dir_entry_mut(&fs->dir, idx);
    assert(e->block_count > 0);
    
    // Corromper un byte del archivo (cambiar 8 bits)
    int block = e->blocks[0];
    int offset = fs->sb.data_offset + block * fs->sb.block_size;
    for (int i = 0; i < 8; i++) {
        int x = (offset + i) % fs->img->width;
        int y = (offset + i) / fs->img->width;
        int old_bit = pbm_get_pixel(fs->img, x, y);
        pbm_set_pixel(fs->img, x, y, !old_bit);
    }
    
    // Verificar que se detecta la corrupción
    int rc = fs_check_integrity(fs);
    printf("Integridad después de corrupción de datos: %d\n", rc);
    assert(rc < 0);
    fs_destroy(fs);
    
    // === Prueba de corrupción de metadatos ===
    fs = fs_load(TEST_PBM_FILE);
    assert(fs);
    
    // Corromper el campo 'size' en la entrada del directorio
    idx = dir_find(&fs->dir, filename);
    assert(idx >= 0);
    e = dir_entry_mut(&fs->dir, idx);
    
    // Guardar el tamaño original
    uint32_t original_size = e->size;
    
    // Cambiar el tamaño a un valor incorrecto
    e->size = original_size + 100;
    
    // Verificar que se detecta la corrupción
    rc = fs_check_integrity(fs);
    printf("Integridad después de corrupción de metadatos: %d\n", rc);
    assert(rc < 0);
    fs_destroy(fs);
    
    // Limpieza
    remove(TEST_PBM_FILE);
    printf("✔ test_fsck\n");
}

int main(void) {
    srand(time(NULL)); // Inicializar semilla aleatoria
    
    // Pruebas básicas
    test_pbm_manager();
    test_block_manager();
    test_superblock();
    test_directory();
    
    // Pruebas con sistema de archivos
    printf("\n--- test_file_operations ---\n");
    FSImage *fs1 = fs_create(TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE);
    assert(fs1);
    test_file_operations(fs1);
    fs_destroy(fs1);
    
    printf("\n--- test_multiple_files ---\n");
    FSImage *fs2 = fs_create(TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE);
    assert(fs2);
    test_multiple_files(fs2);
    fs_destroy(fs2);
    
    printf("\n--- test_boundaries ---\n");
    FSImage *fs3 = fs_create(TEST_WIDTH, TEST_HEIGHT, TEST_BLOCK_SIZE);
    assert(fs3);
    test_boundaries(fs3);
    fs_destroy(fs3);
    
    // Pruebas de persistencia e integridad
    test_persistence();
    test_fsck();
    
    printf("\n🎉 ¡Todas las pruebas pasaron!\n");
    return 0;
}