/**
 * Módulo de Gestión de Procesos Receptores
 * 
 * Este módulo maneja el registro y desregistro de procesos receptores
 * en la memoria compartida, así como el seguimiento de sus estadísticas.
 * Proporciona funciones para mantener un control centralizado de los
 * receptores activos y su información relevante.
 */

#include <stdio.h>
#include <string.h>
#include "process_manager.h"
#include "constants.h"

/**
 * @brief Registra un nuevo proceso receptor
 * 
 * Registra el PID de un nuevo proceso receptor en la memoria compartida
 * y actualiza los contadores de receptores activos y totales.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del proceso receptor a registrar
 * @param sem_global Semáforo global para sincronización
 * @return SUCCESS si se registró correctamente, ERROR en caso contrario
 */
int register_receptor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (!shm || !sem_global) return ERROR;
    sem_wait(sem_global);

    int ok = 0;
    for (int i = 0; i < 100; i++) {
        if (shm->receptor_pids[i] == 0) {
            shm->receptor_pids[i] = pid;
            ok = 1;
            break;
        }
    }
    if (ok) {
        shm->active_receptores++;
        shm->total_receptores++;
        printf(GREEN "[RECEPTOR %d] Registrado (%d activos)\n" RESET, pid, shm->active_receptores);
    }

    sem_post(sem_global);
    return ok ? SUCCESS : ERROR;
}

/**
 * @brief Elimina el registro de un proceso receptor
 * 
 * Elimina el PID de un proceso receptor de la memoria compartida
 * y actualiza el contador de receptores activos.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del proceso receptor a desregistrar
 * @param sem_global Semáforo global para sincronización
 * @return SUCCESS si se desregistró correctamente, ERROR en caso contrario
 */
int unregister_receptor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (!shm || !sem_global) return ERROR;
    sem_wait(sem_global);

    int found = 0;
    for (int i = 0; i < 100; i++) {
        if (shm->receptor_pids[i] == pid) {
            shm->receptor_pids[i] = 0;
            found = 1;
            break;
        }
    }
    if (found) {
        shm->active_receptores--;
        printf(YELLOW "[RECEPTOR %d] Desregistrado (%d activos restantes)\n" RESET,
               pid, shm->active_receptores);
    }

    sem_post(sem_global);
    return found ? SUCCESS : ERROR;
}

/**
 * @brief Guarda las estadísticas de un receptor al finalizar
 * 
 * Almacena en la memoria compartida las estadísticas de ejecución
 * de un proceso receptor, incluyendo caracteres procesados y tiempos
 * de inicio y fin.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del proceso receptor
 * @param chars_received Número de caracteres procesados por el receptor
 * @param start_time Tiempo de inicio del proceso
 * @param end_time Tiempo de finalización del proceso
 * @param sem_global Semáforo global para sincronización
 */
void save_receptor_stats(SharedMemory* shm, pid_t pid, int chars_received,
                        time_t start_time, time_t end_time, sem_t* sem_global) {
    if (!shm || !sem_global) return;
    
    sem_wait(sem_global);
    
    if (shm->receptor_stats_count < 100) {
        int idx = shm->receptor_stats_count;
        shm->receptor_stats[idx].pid = pid;
        shm->receptor_stats[idx].chars_processed = chars_received;
        shm->receptor_stats[idx].start_time = start_time;
        shm->receptor_stats[idx].end_time = end_time;
        shm->receptor_stats_count++;
    }
    
    sem_post(sem_global);
}