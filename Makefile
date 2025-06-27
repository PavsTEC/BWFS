CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O2 -I.

# Flags para FUSE 3
FUSE_CFLAGS := $(shell pkg-config fuse3 --cflags)
FUSE_LIBS   := $(shell pkg-config fuse3 --libs)

# Módulos core del FS
MODULES = pbm_manager block_manager superblock directory fs_image
OBJS    = $(MODULES:%=%.o)

# Detecta automáticamente todos los .c bajo test/
TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(TEST_SRCS:.c=.o)

.PHONY: all test clean

# Por defecto compila mkfs.bwfs, fsck.bwfs y mount.bwfs
all: mkfs.bwfs fsck.bwfs mount.bwfs

# Librería estática
libbwfs.a: $(OBJS)
	ar rcs $@ $^

# Regla genérica para compilar .c → .o (incluye test/*.c y mount.bwfs.c)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# mkfs.bwfs
mkfs.bwfs: mkfs.bwfs.o libbwfs.a
	$(CC) $(CFLAGS) -o $@ mkfs.bwfs.o -L. -lbwfs

# fsck.bwfs
fsck.bwfs: fsck.bwfs.o libbwfs.a
	$(CC) $(CFLAGS) -o $@ fsck.bwfs.o -L. -lbwfs

# mount.bwfs (FUSE)
mount.bwfs: mount.bwfs.o libbwfs.a
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) -o $@ mount.bwfs.o \
	    -L. -lbwfs $(FUSE_LIBS)

# Pruebas
test: test_bwfs

test_bwfs: libbwfs.a $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS) -L. -lbwfs
	@echo "\n⚡ Ejecutando pruebas..."
	@./test_bwfs

# Limpieza
clean:
	rm -f $(OBJS) mkfs.bwfs.o fsck.bwfs.o mount.bwfs.o \
	      libbwfs.a mkfs.bwfs fsck.bwfs mount.bwfs \
	      test_bwfs $(TEST_OBJS) test.pbm
