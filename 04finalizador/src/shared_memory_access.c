#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include "shared_memory_access.h"

/* Define la clave para la memoria compartida, debe ser la misma que usa el inicializador */
#define IPC_KEY 0x1234

/**
 * Funciones para manejo de memoria compartida y estadísticas del sistema
 * Este archivo contiene las funciones necesarias para acceder a la memoria compartida
 * y mostrar las estadísticas de ejecución del sistema.
 */

/**
 * @brief Conecta el proceso a la memoria compartida existente
 * 
 * Esta función obtiene acceso a la memoria compartida creada por el inicializador.
 * Utiliza la clave IPC_KEY para identificar el segmento de memoria correcto.
 * 
 * @return SharedMemory* Puntero a la estructura de memoria compartida
 * @exit Termina el programa si hay error al acceder a la memoria
 */
SharedMemory* attach_shared_memory() {
    int shm_id = shmget(IPC_KEY, sizeof(SharedMemory), 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    SharedMemory* shm = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    return shm;
}

/**
 * @brief Desconecta el proceso de la memoria compartida
 * 
 * Esta función libera la conexión del proceso con el segmento de memoria compartida.
 * Debe llamarse antes de finalizar el programa para liberar recursos.
 * 
 * @param shm Puntero a la estructura de memoria compartida a desconectar
 * @exit Termina el programa si hay error al desconectar
 */
void detach_shared_memory(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        perror("shmdt failed");
        exit(1);
    }
}

/**
 * @brief Muestra las estadísticas finales del sistema
 * 
 * Esta función presenta un resumen completo de la ejecución del sistema, incluyendo:
 * - Estadísticas generales (total de caracteres, progreso)
 * - Estadísticas de cada emisor (PID, caracteres procesados, tiempos)
 * - Estadísticas de cada receptor (PID, caracteres procesados, tiempos)
 * 
 * La información se muestra con formato y colores para mejor legibilidad
 * 
 * @param shm Puntero a la estructura de memoria compartida con las estadísticas
 */
void print_statistics(SharedMemory* shm) {
    printf("\033[1;36m╔════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║                ESTADÍSTICAS DEL SISTEMA                    ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════════════════╝\033[0m\n\n");

    // Estadísticas generales
    printf("\033[1;33mEstadísticas Generales:\033[0m\n");
    printf("  Total de caracteres en archivo: %d\n", shm->total_chars_in_file);
    printf("  Total de caracteres procesados: %d\n", shm->total_chars_processed);
    printf("  Porcentaje completado: %.2f%%\n\n", 
           (float)shm->total_chars_processed / shm->total_chars_in_file * 100);

    // Estadísticas de Emisores
    printf("\033[1;32mEstadísticas de Emisores:\033[0m\n");
    printf("  %-10s %-15s %-20s %-20s\n", "PID", "Chars Proc.", "Tiempo Inicio", "Tiempo Fin");
    printf("  %-10s %-15s %-20s %-20s\n", "----------", "---------------", "--------------------", "--------------------");
    
    for (int i = 0; i < shm->emisor_stats_count; i++) {
        ProcessStats stat = shm->emisor_stats[i];
        char start_time[20], end_time[20];
        strftime(start_time, sizeof(start_time), "%H:%M:%S", localtime(&stat.start_time));
        strftime(end_time, sizeof(end_time), "%H:%M:%S", localtime(&stat.end_time));
        printf("  %-10d %-15d %-20s %-20s\n", 
               stat.pid, stat.chars_processed, start_time, end_time);
    }
    printf("\n");

    // Estadísticas de Receptores
    printf("\033[1;35mEstadísticas de Receptores:\033[0m\n");
    printf("  %-10s %-15s %-20s %-20s\n", "PID", "Chars Proc.", "Tiempo Inicio", "Tiempo Fin");
    printf("  %-10s %-15s %-20s %-20s\n", "----------", "---------------", "--------------------", "--------------------");
    
    for (int i = 0; i < shm->receptor_stats_count; i++) {
        ProcessStats stat = shm->receptor_stats[i];
        char start_time[20], end_time[20];
        strftime(start_time, sizeof(start_time), "%H:%M:%S", localtime(&stat.start_time));
        strftime(end_time, sizeof(end_time), "%H:%M:%S", localtime(&stat.end_time));
        printf("  %-10d %-15d %-20s %-20s\n", 
               stat.pid, stat.chars_processed, start_time, end_time);
    }
    printf("\n");
}