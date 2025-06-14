## Kick-Off: Proyecto BWFS (Black & White File System)

### 1. Introducción

El presente proyecto consiste en la implementación de un sistema de archivos llamado **BWFS (Black & White File System)**, cuya información se almacena en imágenes blanco y negro, donde cada píxel representa un bit de información. El objetivo es simular un sistema de archivos funcional en el espacio de usuario utilizando el lenguaje C y la biblioteca FUSE, respetando los principios de persistencia, jerarquía y acceso estructurado.

La estrategia para abordar este proyecto se basa en:

- Representar el almacenamiento físico del FS mediante una o múltiples imágenes en formato `.pbm` internamente pero el output e input será en .png.
- Utilizar bloques de **4 KB** como unidad lógica de almacenamiento.
- Cada bloque se representará mediante **una región de 64x64 píxeles** (4096 bits) dentro de la imagen.
- Implementar los componentes clásicos de un FS: superbloque, bitmap, tabla de i-nodos, bloques de datos.

El FS será **multisegmentado**, es decir, que esté compuesto por varias imágenes `.pbm` numeradas secuencialmente, lo cual permite escalar el sistema según sea necesario.

### 2. Ambiente de Desarrollo

**Sistema Operativo:** Ubuntu 22.04 LTS (o compatible con FUSE)

**Lenguaje de programación:** C (GNU GCC)

**Librerías principales:**

- `libfuse-dev`: para la implementación del sistema de archivos en espacio de usuario.

**Herramientas de desarrollo:**

- Editor: Visual Studio Code con extensiones para C/C++
- Compilador: `gcc`
- Debugger: `gdb`
- Control de versiones: `git`

**Forma de debugging:**

- Impresión por consola con `printf` para trazabilidad.
- Uso de `gdb` para depuración estructurada.
- Visualización del estado del "disco" mediante apertura de los archivos `.pbm` en GIMP o visor de imágenes.

**Flujo de trabajo:**

- Control de versiones mediante `git`, trabajo en ramas.
- Compilación con `Makefile`.
- Testing local antes de cada `commit` importante.

### 3. Control de Versiones

Se utilizará Git como sistema de control de versiones. El repositorio incluirá:

- Código fuente modularizado.
- Documentación en Markdown con Latex.
- Archivos `.pbm` de prueba.

[Enlace al repositorio] (por definir)

### 4. Diagrama y Decisiones de Diseño

#### Estructura general del FS

- **Imagen PBM:** archivo binario blanco y negro.
- **Superbloque:** contiene metainformación del FS (tamaño, cantidad de bloques, ubicaciones).
- **Bitmap:** indica qué bloques están libres/ocupados.
- **Tabla de i-nodos:** registros que apuntan a bloques de datos y almacenan metadatos del archivo.
- **Bloques de datos:** bloques de 4 KB mapeados a regiones de 64x64 dentro de la imagen.

#### Multisegmentación

- El FS podrá estar compuesto por varios archivos `.pbm`:
  - Ejemplo: `bw_0.pbm`, `bw_1.pbm`, `bw_2.pbm`, etc.
  - El superbloque deberá rastrear cuántos segmentos existen y su relación con los bloques lógicos.

#### Decisiones clave:

- Cada imagen tendrá resolución **4096 x 4096**, lo que permite representar **512 bloques de 4 KB**.
- Se usará el formato **ASCII PBM (P1)** por su simplicidad de lectura y edición.
- El acceso a bloques estará abstraído mediante una función que convierta un bloque lógico a coordenadas de píxeles e imagen correspondiente.
- Se cifrará opcionalmente el superbloque utilizando una passphrase del usuario.

### 5. Modularización del Proyecto

| Módulo           | Responsabilidades                                                 | Archivos sugeridos       |
| ---------------- | ----------------------------------------------------------------- | ------------------------ |
| `pbm_manager`    | Leer/escribir `.pbm`, conversión a PNG, acceso a píxeles          | `pbm_manager.c` / `.h`   |
| `block_manager`  | Mapeo de bloques lógicos, bitmap, asignación y liberación         | `block_manager.c` / `.h` |
| `superblock`     | Definición, cifrado y validación del superbloque                  | `superblock.c` / `.h`    |
| `inode_manager`  | Creación y manejo de i-nodos, punteros a bloques                  | `inode_manager.c` / `.h` |
| `directory`      | Asociación nombre ↔ i-nodo, estructura jerárquica de carpetas     | `directory.c` / `.h`     |
| `fs_image`       | Coordinación de múltiples `.pbm`, inicialización y montaje del FS | `fs_image.c` / `.h`      |
| `fuse_interface` | Implementación de funciones requeridas por FUSE                   | `bwfs_fuse.c`            |
| `utils`          | Funciones auxiliares, hashing, validaciones, logs                 | `utils.c` / `.h`         |

#### Diagrama de relación entre módulos (Representación visual)

```
┌────────────────────┐
│    fuse_interface  │ ← conecta el FS al OS
└────────▲───────────┘
         │
         ▼
┌────────────┐
│  fs_image  │ ← módulo coordinador general del FS
└─┬──────────┘
  │
  ├── superblock
  ├── block_manager
  ├── inode_manager
  ├── directory
  └── pbm_manager
```

---

Este Kick-Off define la base sobre la cual se construirá el sistema BWFS, permitiendo modularidad, escalabilidad y claridad en la ejecución del proyecto.

