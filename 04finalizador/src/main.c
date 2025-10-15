#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>

#include "constants.h"
#include "structures.h"
#include "signal_handler.h"
#include "shared_memory_access.h"

/**
 * Finalizador del Sistema IPC
 *
 * - Espera 'q' (bloqueante, sin busy-wait) o señal externa
 * - Marca shutdown_flag y notifica a emisores/receptores (SIGUSR1)
 * - Despierta potenciales bloqueados en semáforos POSIX
 * - Espera a que todos terminen
 * - Imprime estadísticas (con señales bloqueadas para que no se corte)
 */

static void wake_blocked_processes_posix(int buffer_size) {
    sem_t *es = sem_open(SEM_NAME_ENCRYPT_SPACES, 0);
    sem_t *di = sem_open(SEM_NAME_DECRYPT_ITEMS, 0);

    if (es != SEM_FAILED) {
        for (int i = 0; i < buffer_size; i++) sem_post(es);
        sem_close(es);
        printf("  ! Despertados emisores (ENCRYPT_SPACES)\n");
    }
    if (di != SEM_FAILED) {
        for (int i = 0; i < buffer_size; i++) sem_post(di);
        sem_close(di);
        printf("  ! Despertados receptores (DECRYPT_ITEMS)\n");
    }
    fflush(stdout);
}

static void notify_processes(SharedMemory* shm, int* sent_emisores, int* sent_receptores) {
    int se = 0, sr = 0;
    for (int i = 0; i < 100; i++) {
        if (shm->emisor_pids[i] > 0) {
            if (kill(shm->emisor_pids[i], SIGUSR1) == 0) se++;
        }
    }
    for (int i = 0; i < 100; i++) {
        if (shm->receptor_pids[i] > 0) {
            if (kill(shm->receptor_pids[i], SIGUSR1) == 0) sr++;
        }
    }
    *sent_emisores  = se;
    *sent_receptores = sr;
    printf("  • Señales SIGUSR1 enviadas: emisores=%d, receptores=%d\n", se, sr);
    fflush(stdout);
}

// Limpieza FINAL de IPC: despierta bloqueados, elimina semáforos POSIX y SHM System V.
// Llamar a esta función *después* de esperar a que terminen emisores/receptores.
static void final_cleanup_ipc(SharedMemory* shm) {
    printf(BOLD RED "╔══════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD RED "║                    LIMPIEZA DE IPC                   ║\n" RESET);
    printf(BOLD RED "╚══════════════════════════════════════════════════════╝\n" RESET);

    // 1) Despertar posibles procesos bloqueados (usa tu helper local)
    if (shm && shm->buffer_size > 0) {
        wake_blocked_processes_posix(shm->buffer_size);
        printf(YELLOW "  ! Despertados posibles bloqueados (ENCRYPT_SPACES / DECRYPT_ITEMS)\n" RESET);
    }

    // 2) Eliminar semáforos POSIX nombrados directamente (equivalente a borrar /dev/shm/sem.*)
    printf("  → Eliminando semáforos POSIX nombrados...\n");
    int any_err = 0;
    if (sem_unlink(SEM_NAME_GLOBAL_MUTEX)   == -1) any_err = 1;
    if (sem_unlink(SEM_NAME_ENCRYPT_QUEUE)  == -1) any_err = 1;
    if (sem_unlink(SEM_NAME_DECRYPT_QUEUE)  == -1) any_err = 1;
    if (sem_unlink(SEM_NAME_ENCRYPT_SPACES) == -1) any_err = 1;
    if (sem_unlink(SEM_NAME_DECRYPT_ITEMS)  == -1) any_err = 1;

    if (!any_err) printf(GREEN "  ✓ Semáforos POSIX eliminados\n" RESET);
    else          printf(YELLOW "  • Uno o más semáforos ya no existían o no pudieron eliminarse (continuando)\n" RESET);

    // 3) Eliminar SHM System V por clave (0x1234), como en tu make clean-ipc
    printf("  → Eliminando memoria compartida System V (key 0x%04X)...\n", SHM_BASE_KEY);
    int rc = system("ipcrm -M 0x1234 2>/dev/null || true");
    (void)rc; // evitar warning por valor no usado
    printf(GREEN "  ✓ Solicitud de eliminación de SHM enviada (ipcrm)\n" RESET);

    // 4) Limpieza legacy opcional (prefijo sem.sem.ipc_*)
    printf("  → Limpieza legacy en /dev/shm (sem.sem.ipc_*)...\n");
    rc = system("rm -f /dev/shm/sem.sem.ipc_* 2>/dev/null || true");
    (void)rc;

    printf(GREEN "✓ IPC limpiado (SHM y semáforos POSIX)\n" RESET);
}

int main(void) {
    /* Salida sin búfer para que no se “corte” el output */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\033[1;36m╔════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║                     FINALIZADOR                            ║\033[0m\n");
    printf("\033[1;36m╚════════════════════════════════════════════════════════════╝\033[0m\n\n");

    setup_signal_handlers();
    if (setup_keyboard_input() < 0) {
        fprintf(stderr, "Error configurando entrada de teclado\n");
        return 1;
    }

    SharedMemory* shm = attach_shared_memory();
    if (!shm) {
        fprintf(stderr, "No se pudo adjuntar a la SHM\n");
        cleanup_keyboard();
        return 1;
    }

    /* Espera bloqueante hasta 'q' o señal (sin busy-wait) */
    (void)wait_for_quit_or_signal();

    /* Activar flag de finalización */
    shm->shutdown_flag = 1;
    printf("\033[1;33m→ Solicitando finalización de procesos...\033[0m\n");

    /* Notificar procesos y despertar bloqueados por semáforos POSIX */
    int se = 0, sr = 0;
    notify_processes(shm, &se, &sr);
    wake_blocked_processes_posix(shm->buffer_size);

    /* Esperar a que todos terminen */
    while (shm->active_emisores > 0 || shm->active_receptores > 0) {
        printf("\033[1;34m→ Esperando finalización (%d emisores, %d receptores activos)\033[0m\r",
               shm->active_emisores, shm->active_receptores);
        fflush(stdout);
        sleep(1);
    }
    printf("\n\033[1;32m✓ Todos los procesos han finalizado\033[0m\n\n");

    /* Bloquear señales mientras imprimimos estadísticas para que no se corte */
    sigset_t set, oldset;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    print_statistics(shm);
    sleep(5);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    /* Limpiar y salir */
    printf("\n\033[1;33m→ Limpiando recursos...\033[0m\n");
    cleanup_keyboard();
    final_cleanup_ipc(shm);
    printf("\033[1;32m✓ Finalización completada\033[0m\n");
    fflush(stdout);
    return 0;
}