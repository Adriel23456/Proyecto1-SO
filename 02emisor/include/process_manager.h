#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include <semaphore.h>
#include "structures.h"

int get_next_text_index(SharedMemory* shm, sem_t* sem_global);
int register_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global);
int unregister_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global);

// NUEVO: Guardar estadísticas al finalizar
void save_emisor_stats(SharedMemory* shm, pid_t pid, int chars_sent, 
                       time_t start_time, time_t end_time, sem_t* sem_global);

#endif