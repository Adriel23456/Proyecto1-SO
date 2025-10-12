#ifndef SHARED_MEMORY_INIT_H
#define SHARED_MEMORY_INIT_H

#include <sys/types.h>
#include "structures.h"

/*
 * Creación y gestión de la memoria compartida:
 *  - create_shared_memory: reserva el segmento con todas las regiones necesarias.
 *  - attach_shared_memory / detach_shared_memory: adjunta/desadjunta el segmento.
 *  - cleanup_shared_memory: elimina el segmento (solo debe usarlo el finalizador).
 *  - initialize_buffer_slots / copy_file_to_shared_memory: inicialización de datos.
 *  - get_buffer_pointer / get_file_data_pointer: accesos convenientes por offset.
 */
SharedMemory* create_shared_memory(int buffer_size, int file_size);
SharedMemory* attach_shared_memory(key_t key);
int  detach_shared_memory(SharedMemory* shm);
int  cleanup_shared_memory(SharedMemory* shm);

void initialize_buffer_slots(SharedMemory* shm, int buffer_size);
void copy_file_to_shared_memory(SharedMemory* shm, unsigned char* file_data, int file_size);

CharacterSlot*   get_buffer_pointer(SharedMemory* shm);
unsigned char*   get_file_data_pointer(SharedMemory* shm);

#endif // SHARED_MEMORY_INIT_H
