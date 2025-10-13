// src/output_file.c
// NO definir _POSIX_C_SOURCE ni _DEFAULT_SOURCE aquí (ya están en CFLAGS del Makefile)
#include "output_file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static int ensure_dir(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        errno = ENOTDIR;
        return -1;
    }
    return mkdir(dir, 0777);
}

// Versión segura de basename que no modifica el input
static void safe_basename(const char* path, char* out, size_t outsz) {
    if (!path || !out || outsz == 0) return;
    
    // Encontrar la última barra
    const char* last_slash = strrchr(path, '/');
    const char* name = last_slash ? (last_slash + 1) : path;
    
    // Si está vacío o es solo "/", usar "output"
    if (!name[0]) {
        strncpy(out, "output", outsz - 1);
        out[outsz - 1] = '\0';
        return;
    }
    
    strncpy(out, name, outsz - 1);
    out[outsz - 1] = '\0';
}

int open_output_file(const char* shm_input_filename,
                     int file_size,
                     char* out_path,
                     size_t out_path_sz) {
    if (!shm_input_filename || !out_path || out_path_sz < 8) {
        errno = EINVAL; return -1;
    }

    // Directorio de salida configurable por variable de entorno
    const char* dir = getenv("RECEPTOR_OUT_DIR");
    if (!dir || !*dir) dir = "./out";

    if (ensure_dir(dir) == -1 && errno != EEXIST) {
        // si falló crear ./out (o RECEPTOR_OUT_DIR), usamos el cwd como fallback
        dir = ".";
    }

    char base[PATH_MAX + 1];
    safe_basename(shm_input_filename, base, sizeof(base));
    
    // Si no hay basename válido, usar "output"
    if (!base[0]) {
        strncpy(base, "output", sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
    }

    // Ruta final preferida: <dir>/<basename>.dec.bin
    int need = snprintf(out_path, out_path_sz, "%s/%s.dec.bin", dir, base);
    if (need < 0 || (size_t)need >= out_path_sz) {
        errno = ENAMETOOLONG;
        return -1;
    }

    int fd = open(out_path, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        // último fallback: ./<basename>.dec.bin
        need = snprintf(out_path, out_path_sz, "./%s.dec.bin", base);
        if (need < 0 || (size_t)need >= out_path_sz) {
            errno = ENAMETOOLONG; return -1;
        }
        fd = open(out_path, O_CREAT | O_RDWR, 0666);
        if (fd == -1) return -1;
    }

    // Pre-dimensionar para permitir escrituras aleatorias con pwrite
    if (file_size > 0) {
        if (ftruncate(fd, file_size) == -1) {
            // No es fatal: seguimos, pero dejamos el aviso
            fprintf(stderr, "WARN: ftruncate('%s', %d) falló: %s\n",
                    out_path, file_size, strerror(errno));
        }
    }
    return fd;
}

int write_decoded_char(int fd, int index, unsigned char ch) {
    if (fd < 0 || index < 0) { errno = EINVAL; return -1; }
    ssize_t w = pwrite(fd, &ch, 1, (off_t)index);
    return (w == 1) ? 0 : -1;
}

int close_output_file(int fd) {
    if (fd >= 0) return close(fd);
    return 0;
}