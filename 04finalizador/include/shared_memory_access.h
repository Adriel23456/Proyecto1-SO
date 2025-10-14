#ifndef SHARED_MEMORY_ACCESS_H
#define SHARED_MEMORY_ACCESS_H

#include "structures.h"

SharedMemory* attach_shared_memory(void);
void detach_shared_memory(SharedMemory* shm);
void print_statistics(SharedMemory* shm);

#endif // SHARED_MEMORY_ACCESS_H