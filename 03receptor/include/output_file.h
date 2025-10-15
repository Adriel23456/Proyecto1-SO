#ifndef OUTPUT_FILE_H
#define OUTPUT_FILE_H

#include <stddef.h>

// Abre/crea el archivo de salida en ./out/<basename>.dec.txt
// - Si RECEPTOR_OUT_DIR está definido, usa ese directorio.
// - Pre-dimensiona el archivo a file_size (ftruncate) para escritura aleatoria.
// - Devuelve el fd o -1 en error. out_path se llena con la ruta final usada.
int open_output_file(const char* shm_input_filename,
                     int file_size,
                     char* out_path,
                     size_t out_path_sz);

// Escribe un byte en la posición 'index' (seguro entre procesos).
int write_decoded_char(int fd, int index, unsigned char ch);

// Cierra el descriptor.
int close_output_file(int fd);

#endif // OUTPUT_FILE_H
