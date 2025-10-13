#include <stdio.h>
#include <string.h>
#include "process_manager.h"
#include "constants.h"

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

// NUEVO: Guardar estadísticas del receptor al finalizar
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