#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include <semaphore.h>
#include "structures.h"

int register_receptor(SharedMemory* shm, pid_t pid, sem_t* sem_global);
int unregister_receptor(SharedMemory* shm, pid_t pid, sem_t* sem_global);

#endif // PROCESS_MANAGER_H
