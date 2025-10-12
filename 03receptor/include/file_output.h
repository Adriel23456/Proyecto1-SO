#ifndef FILE_OUTPUT_H
#define FILE_OUTPUT_H

#include <stddef.h>
#include "structures.h"

/* Manejo del archivo .bin de salida en tiempo real.
   - El nombre se deriva de shm->input_filename: "<nombre>.dec.bin"
   - Se garantiza tamaño = total_chars_in_file mediante ftruncate.
   - Escritura posicionada con pwrite para permitir N receptores en paralelo. */

int  open_output_bin(SharedMemory* shm, int* out_fd, char* out_path, size_t path_len);
int  write_char_at_index(int fd, int text_index, char value);
void close_output_bin(int fd);

#endif // FILE_OUTPUT_H
