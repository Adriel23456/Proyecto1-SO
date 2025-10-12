#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include <sys/types.h>
#include <time.h>
#include "structures.h"

// Funciones de conexión a memoria compartida
SharedMemory* attach_shared_memory(key_t key);
int detach_shared_memory(SharedMemory* shm);

// Funciones de acceso a punteros
CharacterSlot* get_buffer_pointer(SharedMemory* shm);
unsigned char* get_file_data_pointer(SharedMemory* shm);

// Funciones de lectura/escritura
char read_char_at_position(SharedMemory* shm, int position);
void store_character(SharedMemory* shm, int slot_index, unsigned char encrypted_char, 
                    int text_index, pid_t emisor_pid);
int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* slot_info);

// Funciones de verificación
int check_shared_memory_health(SharedMemory* shm);
void print_shared_memory_status(SharedMemory* shm);

#endif // SHARED_MEMORY_ACCESS_H