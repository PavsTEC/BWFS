CC       := gcc
CFLAGS   := -Wall -Wextra -I.

# Fuentes de los módulos
PBM_SRCS   := pbm_manager.c
BM_SRCS    := block_manager.c
SB_SRCS    := superblock.c
IN_SRCS    := inode_manager.c
DIR_SRCS   := directory.c
FS_SRCS    := fs_image.c

# Objetos
PBM_OBJS   := $(PBM_SRCS:.c=.o)
BM_OBJS    := $(BM_SRCS:.c=.o)
SB_OBJS    := $(SB_SRCS:.c=.o)
IN_OBJS    := $(IN_SRCS:.c=.o)
DIR_OBJS   := $(DIR_SRCS:.c=.o)
FS_OBJS    := $(FS_SRCS:.c=.o)

# Bibliotecas estáticas
LIBPBM     := libpbm.a
LIBBM      := libbm.a
LIBSB      := libsb.a
LIBIN      := libin.a
LIBDIR     := libdir.a
LIBFS      := libfs.a

# Ejecutables de prueba
TEST_PBM    := test_pbm_manager
TEST_BLOCK  := test_block_manager
TEST_SUPER  := test_superblock
TEST_INODE  := test_inode_manager
TEST_DIR    := test_directory
TEST_FS     := test_fs_image
TEST_LOAD   := test_fs_load

# Ruta a carpeta de tests
TEST_DIR_PATH := tests

.PHONY: all test clean bwfs_fuse

# Objetivo por defecto: compilar todas las bibliotecas
all: $(LIBPBM) $(LIBBM) $(LIBSB) $(LIBIN) $(LIBDIR) $(LIBFS)

# Construcción de bibliotecas estáticas
$(LIBPBM): $(PBM_OBJS)
	ar rcs $@ $^

$(LIBBM): $(BM_OBJS)
	ar rcs $@ $^

$(LIBSB): $(SB_OBJS)
	ar rcs $@ $^

$(LIBIN): $(IN_OBJS)
	ar rcs $@ $^

$(LIBDIR): $(DIR_OBJS)
	ar rcs $@ $^

$(LIBFS): $(FS_OBJS)
	ar rcs $@ $^

# Regla genérica para compilar .o desde .c
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# ----------------------------------------
# Pruebas unitarias
# ----------------------------------------
test: $(TEST_PBM) $(TEST_BLOCK) $(TEST_SUPER) $(TEST_INODE) $(TEST_DIR) $(TEST_FS) $(TEST_LOAD)

$(TEST_PBM): $(LIBPBM) $(TEST_DIR_PATH)/test_pbm_manager.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o $(TEST_DIR_PATH)/test_pbm_manager.c && ./$(TEST_PBM)

$(TEST_BLOCK): $(LIBPBM) $(LIBBM) $(TEST_DIR_PATH)/test_block_manager.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o block_manager.o $(TEST_DIR_PATH)/test_block_manager.c && ./$(TEST_BLOCK)

$(TEST_SUPER): $(LIBPBM) $(LIBSB) $(TEST_DIR_PATH)/test_superblock.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o superblock.o $(TEST_DIR_PATH)/test_superblock.c && ./$(TEST_SUPER)

$(TEST_INODE): $(LIBPBM) $(LIBIN) $(TEST_DIR_PATH)/test_inode_manager.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o inode_manager.o $(TEST_DIR_PATH)/test_inode_manager.c && ./$(TEST_INODE)

$(TEST_DIR): $(LIBPBM) $(LIBDIR) $(TEST_DIR_PATH)/test_directory.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o directory.o $(TEST_DIR_PATH)/test_directory.c && ./$(TEST_DIR)

$(TEST_FS): $(LIBPBM) $(LIBBM) $(LIBSB) $(LIBFS) $(TEST_DIR_PATH)/test_fs_image.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o block_manager.o superblock.o fs_image.o $(TEST_DIR_PATH)/test_fs_image.c && ./$(TEST_FS)

$(TEST_LOAD): $(LIBPBM) $(LIBBM) $(LIBSB) $(LIBFS) $(TEST_DIR_PATH)/test_fs_load.c
	$(CC) $(CFLAGS) -o $@ pbm_manager.o block_manager.o superblock.o fs_image.o $(TEST_DIR_PATH)/test_fs_load.c && ./$(TEST_LOAD)

# ----------------------------------------
# Objetivo para compilar el binario FUSE
# ----------------------------------------
bwfs_fuse: bwfs_fuse.o pbm_manager.o block_manager.o superblock.o fs_image.o
	$(CC) $(CFLAGS) $(shell pkg-config fuse3 --cflags) \
	    -o $@ bwfs_fuse.o pbm_manager.o block_manager.o superblock.o fs_image.o \
	    $(shell pkg-config fuse3 --libs)

# Regla para compilar el objeto de FUSE
bwfs_fuse.o: bwfs_fuse.c fs_image.h
	$(CC) $(CFLAGS) $(shell pkg-config fuse3 --cflags) -c bwfs_fuse.c

# ----------------------------------------
# Limpieza
# ----------------------------------------
clean:
	rm -f *.o *.a $(TEST_PBM) $(TEST_BLOCK) $(TEST_SUPER) $(TEST_INODE) $(TEST_DIR) $(TEST_FS) $(TEST_LOAD) bwfs_fuse
