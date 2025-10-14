#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include "process_manager.h"
#include "constants.h"

/**
 * Módulo de Gestión de Procesos Emisores
 * 
 * Este módulo maneja el registro y control de procesos emisores,
 * incluyendo su registro/desregistro del sistema, asignación de
 * índices de texto, y recolección de estadísticas de ejecución.
 */

/**
 * @brief Obtiene el siguiente índice de texto a procesar
 * 
 * De manera atómica, obtiene y actualiza el índice actual del texto
 * que debe ser procesado. Esto asegura que cada carácter sea
 * procesado exactamente una vez.
 * 
 * @param shm Puntero a la memoria compartida
 * @param sem_global Semáforo para sincronización global
 * @return Siguiente índice a procesar, o -1 si ya no hay caracteres
 */
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

/**
 * @brief Registra un nuevo proceso emisor en el sistema
 * 
 * Añade un nuevo emisor a la lista de procesos activos y
 * actualiza los contadores correspondientes. El registro
 * se realiza de manera atómica usando el semáforo global.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del emisor a registrar
 * @param sem_global Semáforo para sincronización global
 * @return SUCCESS si el registro fue exitoso, ERROR en caso contrario
 */
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

/**
 * @brief Elimina un emisor del registro del sistema
 * 
 * Remueve un emisor de la lista de procesos activos y
 * actualiza los contadores. La operación es atómica
 * gracias al semáforo global.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del emisor a eliminar
 * @param sem_global Semáforo para sincronización global
 * @return SUCCESS si se encontró y eliminó el emisor, ERROR en caso contrario
 */
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

/**
 * @brief Guarda las estadísticas de un emisor que finaliza
 * 
 * Almacena información estadística sobre la ejecución del emisor,
 * incluyendo cantidad de caracteres procesados y tiempos de
 * ejecución. Estas estadísticas son utilizadas por el finalizador
 * para mostrar el resumen del sistema.
 * 
 * @param shm Puntero a la memoria compartida
 * @param pid PID del emisor
 * @param chars_sent Número de caracteres procesados
 * @param start_time Tiempo de inicio del emisor
 * @param end_time Tiempo de finalización del emisor
 * @param sem_global Semáforo para sincronización global
 */
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