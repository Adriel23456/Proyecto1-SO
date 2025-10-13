#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "process_manager.h"
#include "constants.h"

int get_next_text_index(SharedMemory* shm, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return -1;
    
    int index;
    sem_wait(sem_global);
    index = shm->current_txt_index;
    if (index < shm->total_chars_in_file) {
        shm->current_txt_index++;
        shm->total_chars_processed++;
    }
    sem_post(sem_global);
    
    return index;
}

int register_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return ERROR;
    
    sem_wait(sem_global);
    
    int registered = 0;
    for (int i = 0; i < 100; i++) {
        if (shm->emisor_pids[i] == 0) {
            shm->emisor_pids[i] = pid;
            registered = 1;
            break;
        }
    }
    
    if (registered) {
        shm->active_emisores++;
        shm->total_emisores++;
        printf(GREEN "[EMISOR %d] Registrado exitosamente (%d activos)\n" RESET, 
               pid, shm->active_emisores);
    }
    
    sem_post(sem_global);
    return registered ? SUCCESS : ERROR;
}

int unregister_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return ERROR;
    
    sem_wait(sem_global);
    
    int found = 0;
    for (int i = 0; i < 100; i++) {
        if (shm->emisor_pids[i] == pid) {
            shm->emisor_pids[i] = 0;
            found = 1;
            break;
        }
    }
    
    if (found) {
        shm->active_emisores--;
        printf(YELLOW "[EMISOR %d] Desregistrado (%d activos restantes)\n" RESET,
               pid, shm->active_emisores);
    }
    
    sem_post(sem_global);
    return found ? SUCCESS : ERROR;
}

// NUEVO: Guardar estadísticas del emisor al finalizar
void save_emisor_stats(SharedMemory* shm, pid_t pid, int chars_sent, 
                       time_t start_time, time_t end_time, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return;
    
    sem_wait(sem_global);
    
    if (shm->emisor_stats_count < 100) {
        int idx = shm->emisor_stats_count;
        shm->emisor_stats[idx].pid = pid;
        shm->emisor_stats[idx].chars_processed = chars_sent;
        shm->emisor_stats[idx].start_time = start_time;
        shm->emisor_stats[idx].end_time = end_time;
        shm->emisor_stats_count++;
    }
    
    sem_post(sem_global);
}