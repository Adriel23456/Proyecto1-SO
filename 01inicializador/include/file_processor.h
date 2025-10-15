#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include <stddef.h>
#include "structures.h"

/*
 * Procesamiento de archivos:
 *  - process_input_file: lee el archivo de entrada completo en memoria y retorna buffer.
 *  - print_file_statistics: imprime métricas básicas del contenido.
 */
unsigned char* process_input_file(const char* filename, size_t* file_size);
void print_file_statistics(const unsigned char* data, size_t size);

/*
 * Acceso a datos del archivo mapeado en SHM:
 *  - read_char_at_position: lee un carácter en la posición solicitada.
 *  - validate_file_in_shared_memory: verificación básica de integridad.
 */
char read_char_at_position(SharedMemory* shm, int position);
int  validate_file_in_shared_memory(SharedMemory* shm);

#endif // FILE_PROCESSOR_H
