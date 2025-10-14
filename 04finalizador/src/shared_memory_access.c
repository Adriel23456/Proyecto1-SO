#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include "shared_memory_access.h"

#define IPC_KEY 0x1234

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

void detach_shared_memory(SharedMemory* shm) {
    if (shmdt(shm) == -1) {
        perror("shmdt failed");
        exit(1);
    }
}

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