#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include <sys/types.h>
#include "structures.h"

SharedMemory* attach_shared_memory(key_t key);
int detach_shared_memory(SharedMemory* shm);
char read_char_at_position(SharedMemory* shm, int position);
void store_character(SharedMemory* shm, int slot_index, unsigned char encrypted_char, 
                    int text_index, pid_t emisor_pid);

#endif