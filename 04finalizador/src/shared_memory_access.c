#include <stdio.h>
#include <sys/shm.h>
#include <time.h>
#include "shared_memory_access.h"
#include "constants.h"   // SHM_BASE_KEY y colores

/**
 * Funciones para manejo de memoria compartida y estadísticas del sistema
 */

SharedMemory* attach_shared_memory(void) {
    int shm_id = shmget(SHM_BASE_KEY, sizeof(SharedMemory), 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        return NULL;
    }
    SharedMemory* shm = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shm == (void*)-1) {
        perror("shmat failed");
        return NULL;
    }
    return shm;
}

void detach_shared_memory(SharedMemory* shm) {
    if (!shm) return;
    if (shmdt(shm) == -1) {
        perror("shmdt failed");
    }
}

static void fmt_time(char* out, size_t n, time_t t) {
    if (!out || n == 0) return;
    struct tm tmp;
    localtime_r(&t, &tmp);
    strftime(out, n, "%H:%M:%S", &tmp);
}

void print_statistics(SharedMemory* shm) {
    if (!shm) return;

    /* Snapshots para consistencia visual */
    const int total_file  = shm->total_chars_in_file;
    const int total_proc  = shm->total_chars_processed;
    const int act_e       = shm->active_emisores;
    const int tot_e       = shm->total_emisores;
    const int act_r       = shm->active_receptores;
    const int tot_r       = shm->total_receptores;
    const int buf_sz      = shm->buffer_size;

    int emisores_n   = shm->emisor_stats_count;
    int receptores_n = shm->receptor_stats_count;

    /* Límites explícitos en líneas separadas para evitar -Wmisleading-indentation */
    if (emisores_n < 0)  emisores_n = 0;
    if (emisores_n > 100) emisores_n = 100;

    if (receptores_n < 0)  receptores_n = 0;
    if (receptores_n > 100) receptores_n = 100;

    printf("\033[1;36m╔════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║                ESTADÍSTICAS DEL SISTEMA                    ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════════════════╝\033[0m\n\n");

    /* Generales */
    printf("\033[1;33mEstadísticas Generales:\033[0m\n");
    printf("  Total de caracteres en archivo:  %d\n", total_file);
    printf("  Total de caracteres procesados:  %d\n", total_proc);
    printf("  Caracteres en memoria compartida: %d\n",
           shm->decrypt_queue.size + shm->encrypt_queue.size);
    if (total_file > 0) {
        printf("  Porcentaje completado: %.2f%%\n",
               (float)total_proc / (float)total_file * 100.0f);
    } else {
        printf("  Porcentaje completado: N/A\n");
    }
    fflush(stdout);

    /* Estado de procesos */
    printf("\n\033[1;34mEstado de Procesos:\033[0m\n");
    printf("  Emisores activos:  %d / %d (total histórico)\n", act_e, tot_e);
    printf("  Receptores activos: %d / %d (total histórico)\n", act_r, tot_r);
    fflush(stdout);

    /* Uso (estimado) */
    size_t buffer_bytes = (size_t)buf_sz * sizeof(CharacterSlot);
    size_t queue_bytes  = 2ULL * (size_t)buf_sz * sizeof(SlotRef);
    size_t stats_bytes  = (sizeof(ProcessStats) * 200);
    size_t total_bytes  = sizeof(SharedMemory) + buffer_bytes + queue_bytes + stats_bytes;

    printf("\n\033[1;36mUso de Memoria:\033[0m\n");
    printf("  Buffer de caracteres: %zu bytes\n", buffer_bytes);
    printf("  Colas de slots:      %zu bytes\n", queue_bytes);
    printf("  Estadísticas:        %zu bytes\n", stats_bytes);
    printf("  Total utilizado:     %zu bytes (%.2f MB)\n",
           total_bytes, (float)total_bytes / (1024.0f * 1024.0f));
    fflush(stdout);

    /* Emisores */
    printf("\033[1;32mEstadísticas de Emisores:\033[0m\n");
    printf("  %-10s %-15s %-20s %-20s\n", "PID", "Chars Proc.", "Tiempo Inicio", "Tiempo Fin");
    printf("  %-10s %-15s %-20s %-20s\n", "----------", "---------------", "--------------------", "--------------------");
    for (int i = 0; i < emisores_n; i++) {
        ProcessStats st = shm->emisor_stats[i];
        char a[20]={0}, b[20]={0};
        fmt_time(a, sizeof(a), st.start_time);
        fmt_time(b, sizeof(b), st.end_time);
        printf("  %-10d %-15d %-20s %-20s\n", st.pid, st.chars_processed, a, b);
    }
    printf("\n");
    fflush(stdout);

    /* Receptores (encabezado SIEMPRE) */
    printf("\033[1;35mEstadísticas de Receptores:\033[0m\n");
    printf("  %-10s %-15s %-20s %-20s\n", "PID", "Chars Proc.", "Tiempo Inicio", "Tiempo Fin");
    printf("  %-10s %-15s %-20s %-20s\n", "----------", "---------------", "--------------------", "--------------------");
    for (int i = 0; i < receptores_n; i++) {
        ProcessStats st = shm->receptor_stats[i];
        char a[20]={0}, b[20]={0};
        fmt_time(a, sizeof(a), st.start_time);
        fmt_time(b, sizeof(b), st.end_time);
        printf("  %-10d %-15d %-20s %-20s\n", st.pid, st.chars_processed, a, b);
    }
    printf("\n");
    fflush(stdout);
}
