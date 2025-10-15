#include "output_file.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

#include "constants.h"

/**
 * Implementación de utilidades de archivo de salida para el Receptor
 * 
 * Cambios clave:
 * - El archivo generado ahora es texto: <basename>.dec.txt
 * - Se mantiene escritura posicional con pwrite() para soportar
 *   múltiples receptores escribiendo de forma asíncrona.
 * - Se "modifica la metadata" del archivo con ftruncate()/posix_fallocate()
 *   para reservar el tamaño total (inserciones por offset válidas).
 */

/**
 * @brief Obtiene el nombre base (basename) de una ruta
 * 
 * No usa basename(3) para evitar modificar el buffer original o
 * dependencias de comportamiento. Copia la porción después del
 * último '/' a 'dst'.
 * 
 * @param path Ruta de entrada (no modificada)
 * @param dst  Buffer de salida
 * @param n    Tamaño de 'dst'
 * @return 0 en éxito, -1 en error
 */
static int path_basename(const char* path, char* dst, size_t n) {
    if (!path || !dst || n == 0) return -1;
    const char* p = strrchr(path, '/');
    const char* base = p ? (p + 1) : path;
    if (strlen(base) + 1 > n) return -1;
    strncpy(dst, base, n);
    dst[n - 1] = '\0';
    return 0;
}

/**
 * @brief Concatena ruta con seguridad de tamaño
 */
static int join_path(const char* dir, const char* name, char* out, size_t out_sz) {
    if (!dir || !name || !out || out_sz == 0) return -1;
    size_t dlen = strlen(dir);
    int need_slash = (dlen > 0 && dir[dlen - 1] != '/');
    int written = snprintf(out, out_sz, "%s%s%s", dir, need_slash ? "/" : "", name);
    return (written > 0 && (size_t)written < out_sz) ? 0 : -1;
}

/**
 * @brief Asegura que el directorio exista (mkdir -p básico)
 */
static void ensure_dir_exists(const char* dir) {
    if (!dir || !*dir) return;
    // Intentar crear; si ya existe no es error
    mkdir(dir, 0777);
}

/**
 * @brief Construye la ruta final de salida: <OUTDIR>/<basename>.dec.txt
 */
static int build_output_path(const char* shm_input_filename, char* out_path, size_t out_path_sz) {
    char base[NAME_MAX];
    if (path_basename(shm_input_filename, base, sizeof base) != 0) return -1;

    // Armamos "<basename>.txt"
    char fname[NAME_MAX];
    int w = snprintf(fname, sizeof fname, "%s.txt", base);
    if (w <= 0 || (size_t)w >= sizeof fname) return -1;

    // Directorio
    const char* outdir = getenv("RECEPTOR_OUT_DIR");
    if (!outdir || !*outdir) outdir = "./out";
    ensure_dir_exists(outdir);

    return join_path(outdir, fname, out_path, out_path_sz);
}

int open_output_file(const char* shm_input_filename,
                     int file_size,
                     char* out_path,
                     size_t out_path_sz)
{
    if (!shm_input_filename || file_size < 0 || !out_path || out_path_sz == 0) {
        errno = EINVAL;
        return -1;
    }

    if (build_output_path(shm_input_filename, out_path, out_path_sz) != 0) {
        errno = ENAMETOOLONG;
        return -1;
    }

    int fd = open(out_path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        // Mensaje amigable; el caller imprimirá strerror(errno)
        return -1;
    }

    // --- Modificación de metadata para soportar inyecciones posicionales ---
    // Pre-dimensionar: garantiza que los offsets [0..file_size-1] existan.
    if (ftruncate(fd, (off_t)file_size) == -1) {
        // Si falla, seguimos, pero las escrituras podrían crear huecos
        // (sparse file). No se considera fatal para funcionalidad.
    }

    // Si está disponible, intentar reservar físicamente el espacio.
    // Esto evita sorpresas con huecos en sistemas que lo soportan.
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L)
    // posix_fallocate devuelve 0 en éxito y un código de error POSIX si falla.
    // No tratamos la falla como fatal; pwrite seguirá funcionando.
    (void)posix_fallocate(fd, 0, (off_t)file_size);
#endif

    return fd;
}

int write_decoded_char(int fd, int index, unsigned char ch) {
    if (fd < 0 || index < 0) {
        errno = EINVAL;
        return -1;
    }
    ssize_t n = pwrite(fd, &ch, 1, (off_t)index);
    if (n != 1) return -1;
    return 0;
}

int close_output_file(int fd) {
    if (fd < 0) return 0;
    return close(fd);
}