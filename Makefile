CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I.

# Flags para FUSE (ajusta si usas fuse3 u otro)
FUSE_CFLAGS := $(shell pkg-config fuse --cflags)
FUSE_LIBS   := $(shell pkg-config fuse --libs)

# Módulos core del FS
MODULES = pbm_manager block_manager superblock directory fs_image
OBJS    = $(MODULES:%=%.o)

# Detecta automáticamente todos los .c bajo test/
TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(TEST_SRCS:.c=.o)

.PHONY: all test clean

all: bwfs_fuse

# Librería estática
libbwfs.a: $(OBJS)
	ar rcs $@ $^

# Regla genérica para compilar .c → .o (incluye test/*.c)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pruebas
test: test_bwfs

test_bwfs: libbwfs.a $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) -L. -lbwfs
	@echo "\n⚡ Ejecutando pruebas..."
	@./test_bwfs

# Limpieza
clean:
	rm -f $(OBJS) bwfs_fuse.o $(TEST_OBJS) libbwfs.a bwfs_fuse test_bwfs test.pbm
