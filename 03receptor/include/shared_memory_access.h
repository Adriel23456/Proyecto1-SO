#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include <sys/types.h>
#include "structures.h"
#include <sys/types.h>
#include <sys/ipc.h>

SharedMemory* attach_shared_memory(key_t key);
int detach_shared_memory(SharedMemory* shm);

CharacterSlot* get_buffer_pointer(SharedMemory* shm);
unsigned char* get_file_data_pointer(SharedMemory* shm);

int get_slot_info(SharedMemory* shm, int slot_index, CharacterSlot* out);
void clear_slot(SharedMemory* shm, int slot_index); // marcar libre

#endif // SHARED_MEMORY_ACCESS_H