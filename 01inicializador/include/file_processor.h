#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include <stddef.h>       // Para size_t
#include "structures.h"

/*
 * Funciones de procesamiento de archivos:
 *  - process_input_file: lee el .txt de entrada, devuelve un buffer con todo el contenido
 *    y genera un archivo .bin junto al archivo original para verificación.
 *  - write_binary_file: persiste un bloque de datos binarios en disco.
 *  - print_file_statistics: imprime estadísticas básicas del contenido leído.
 *
 * Notas:
 *  - Los tamaños se expresan como size_t para evitar problemas de signed/unsigned.
 *  - Los datos que retornan se deben liberar por el llamador con free().
 */
unsigned char* process_input_file(const char* filename, size_t* file_size);
int write_binary_file(const char* filename, const unsigned char* data, size_t size);
void print_file_statistics(const unsigned char* data, size_t size);

/*
 * Funciones de acceso a datos del archivo en memoria compartida:
 *  - read_char_at_position: obtiene el carácter en la posición dada desde la SHM.
 *  - validate_file_in_shared_memory: verificación simple de integridad.
 */
char read_char_at_position(SharedMemory* shm, int position);
int validate_file_in_shared_memory(SharedMemory* shm);

#endif // FILE_PROCESSOR_H
