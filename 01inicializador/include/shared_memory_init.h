#ifndef SHARED_MEMORY_INIT_H
#define SHARED_MEMORY_INIT_H

#include <sys/types.h>
#include "structures.h"

// Funciones principales de memoria compartida
SharedMemory* create_shared_memory(int buffer_size, int file_size);
SharedMemory* attach_shared_memory(key_t key);
int detach_shared_memory(SharedMemory* shm);
int cleanup_shared_memory(SharedMemory* shm);

// Funciones de inicialización
void initialize_buffer_slots(SharedMemory* shm, int buffer_size);
void copy_file_to_shared_memory(SharedMemory* shm, unsigned char* file_data, int file_size);

// Funciones de acceso a punteros
CharacterSlot* get_buffer_pointer(SharedMemory* shm);
unsigned char* get_file_data_pointer(SharedMemory* shm);

#endif // SHARED_MEMORY_INIT_H