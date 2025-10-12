#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_output.h"
#include "constants.h"

static void build_output_path(const char* input_filename, char* out_path, size_t n) {
    // "<input>.dec.bin"
    snprintf(out_path, n, "%s.dec.bin", input_filename && input_filename[0] ? input_filename : "output");
}

int open_output_bin(SharedMemory* shm, int* out_fd, char* out_path, size_t path_len) {
    if (!shm || !out_fd || !out_path) return ERROR;
    build_output_path(shm->input_filename, out_path, path_len);

    // Abrir/crear con permisos 0666
    int fd = open(out_path, O_WRONLY | O_CREAT, 0666);
    if (fd == -1) {
        fprintf(stderr, RED "[ERROR] No se pudo abrir/crear '%s': %s\n" RESET,
                out_path, strerror(errno));
        return ERROR;
    }

    // Asegurar tamaño del archivo (permite escritura posicional segura)
    if (ftruncate(fd, shm->total_chars_in_file) == -1) {
        fprintf(stderr, YELLOW "[WARN] ftruncate('%s'): %s\n" RESET, out_path, strerror(errno));
        // No es fatal; continuamos (el pwrite ajustará tamaño según FS)
    }

    *out_fd = fd;
    printf(GREEN "✓ Archivo de salida listo: %s (%d bytes)\n" RESET, out_path, shm->total_chars_in_file);
    return SUCCESS;
}

int write_char_at_index(int fd, int text_index, char value) {
    if (fd < 0 || text_index < 0) return ERROR;
    ssize_t wr = pwrite(fd, &value, 1, (off_t)text_index);
    if (wr != 1) return ERROR;
    // Opcional: forzar flush a disco
    // fdatasync(fd);
    return SUCCESS;
}

void close_output_bin(int fd) {
    if (fd >= 0) close(fd);
}
