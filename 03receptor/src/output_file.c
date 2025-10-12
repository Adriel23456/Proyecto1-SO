// src/output_file.c
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "output_file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>   // <-- necesario para getenv

static int ensure_dir(const char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return 0;
        errno = ENOTDIR;
        return -1;
    }
    return mkdir(dir, 0777);
}

static const char* safe_basename(const char* path, char* buf, size_t bufsz) {
    if (!path || !buf || bufsz == 0) { errno = EINVAL; return NULL; }

    size_t n = strnlen(path, PATH_MAX);
    char tmp[PATH_MAX + 1];
    if (n > PATH_MAX) n = PATH_MAX;
    memcpy(tmp, path, n);
    tmp[n] = '\0';

    const char* b = basename(tmp);   // basename puede modificar el buffer
    if (!b) { errno = EINVAL; return NULL; }

    strncpy(buf, b, bufsz - 1);
    buf[bufsz - 1] = '\0';
    return buf;
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
    if (!safe_basename(shm_input_filename, base, sizeof(base))) {
        // fallback por si no hay basename razonable
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
