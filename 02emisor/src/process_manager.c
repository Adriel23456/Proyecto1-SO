#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "process_manager.h"
#include "constants.h"
#include "structures.h"

/*
 * Obtener el siguiente índice del texto atómicamente.
 * Incrementa current_txt_index y retorna el valor anterior.
 */
int get_next_text_index(SharedMemory* shm, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return -1;
    
    int index;
    
    // Sección crítica - acceso atómico
    sem_wait(sem_global);
    index = shm->current_txt_index;
    if (index < shm->total_chars_in_file) {
        shm->current_txt_index++;
        shm->total_chars_processed++;
    }
    sem_post(sem_global);
    
    return index;
}

/*
 * Registrar un nuevo emisor en el sistema.
 */
int register_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return ERROR;
    
    sem_wait(sem_global);
    
    // Buscar slot disponible en array de PIDs
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
    } else {
        fprintf(stderr, RED "[ERROR] No hay espacio para registrar emisor %d\n" RESET, pid);
    }
    
    sem_post(sem_global);
    
    return registered ? SUCCESS : ERROR;
}

/*
 * Desregistrar un emisor del sistema.
 */
int unregister_emisor(SharedMemory* shm, pid_t pid, sem_t* sem_global) {
    if (shm == NULL || sem_global == NULL) return ERROR;
    
    sem_wait(sem_global);
    
    // Buscar y eliminar el PID
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

/*
 * Verificar si un emisor está registrado.
 */
int is_emisor_registered(SharedMemory* shm, pid_t pid) {
    if (shm == NULL) return 0;
    
    for (int i = 0; i < 100; i++) {
        if (shm->emisor_pids[i] == pid) {
            return 1;
        }
    }
    return 0;
}

/*
 * Obtener el número de emisores activos.
 */
int get_active_emisores_count(SharedMemory* shm) {
    if (shm == NULL) return 0;
    return shm->active_emisores;
}

/*
 * Obtener el número de receptores activos.
 */
int get_active_receptores_count(SharedMemory* shm) {
    if (shm == NULL) return 0;
    return shm->active_receptores;
}

/*
 * Verificar si el sistema está en proceso de apagado.
 */
int is_system_shutting_down(SharedMemory* shm) {
    if (shm == NULL) return 1;
    return shm->shutdown_flag;
}

/*
 * Listar todos los emisores activos.
 */
void list_active_emisores(SharedMemory* shm) {
    if (shm == NULL) return;
    
    printf(CYAN "\nEmisores activos:\n" RESET);
    int count = 0;
    
    for (int i = 0; i < 100; i++) {
        if (shm->emisor_pids[i] != 0) {
            printf("  • PID %d\n", shm->emisor_pids[i]);
            count++;
        }
    }
    
    if (count == 0) {
        printf("  (ninguno)\n");
    } else {
        printf("Total: %d emisores\n", count);
    }
}

/*
 * Obtener estadísticas del proceso emisor.
 */
void get_emisor_stats(SharedMemory* shm, pid_t pid, EmitterStats* stats) {
    if (shm == NULL || stats == NULL) return;
    
    memset(stats, 0, sizeof(EmitterStats));
    stats->pid = pid;
    stats->is_active = is_emisor_registered(shm, pid);
    
    // Contar caracteres enviados por este emisor
    CharacterSlot* buffer = (CharacterSlot*)((char*)shm + shm->buffer_offset);
    for (int i = 0; i < shm->buffer_size; i++) {
        if (buffer[i].is_valid && buffer[i].emisor_pid == pid) {
            stats->chars_sent++;
        }
    }
}

/*
 * Imprimir estadísticas de un emisor.
 */
void print_emisor_stats(EmitterStats* stats) {
    if (stats == NULL) return;
    
    printf(CYAN "\n╔══════════════════════════════════════════════════════════╗\n" RESET);
    printf(CYAN "║              ESTADÍSTICAS EMISOR PID %-6d             ║\n", stats->pid);
    printf(CYAN "╚══════════════════════════════════════════════════════════╝\n" RESET);
    
    printf("  • Estado: %s\n", stats->is_active ? "ACTIVO" : "INACTIVO");
    printf("  • Caracteres enviados: %d\n", stats->chars_sent);
}