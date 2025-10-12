#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>
#include <semaphore.h>
#include "structures.h"

// Estructura para estadísticas del emisor
typedef struct {
    pid_t pid;
    int is_active;
    int chars_sent;
} EmitterStats;

// Funciones de gestión de índices
int get_next_text_index(SharedMemory* shm, sem_t* sem_global);

// Funciones de registro
int register_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global);
int unregister_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global);
int is_emisor_registered(SharedMemory* shm, pid_t pid);

// Funciones de consulta
int get_active_emisores_count(SharedMemory* shm);
int get_active_receptores_count(SharedMemory* shm);
int is_system_shutting_down(SharedMemory* shm);

// Funciones de listado y estadísticas
void list_active_emisores(SharedMemory* shm);
void get_emisor_stats(SharedMemory* shm, pid_t pid, EmitterStats* stats);
void print_emisor_stats(EmitterStats* stats);

#endif // PROCESS_MANAGER_H